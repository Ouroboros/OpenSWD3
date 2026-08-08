'use strict';

const EXPECTED_ARCH = 'ia32';
const EXPECTED_IMAGE_BASE = ptr('0x00400000');
const GLYPH_ENTRY = ptr('0x004368D0');
const RENDERER_ROW_BYTES = 0x1c;
const RENDERER_WIDTH = 0xfd0;
const RENDERER_HEIGHT = 0xfd4;
const MAXIMUM_GLYPH_DIMENSION = 64;
const activeGlyphs = new Map();
const reportedFonts = new Set();

const getTextFaceW = new NativeFunction(
    Module.getExportByName('gdi32.dll', 'GetTextFaceW'),
    'int',
    ['pointer', 'int', 'pointer']
);
const getTextMetricsA = new NativeFunction(
    Module.getExportByName('gdi32.dll', 'GetTextMetricsA'),
    'int',
    ['pointer', 'pointer']
);

function hexByte(value) {
    return value.toString(16).padStart(2, '0');
}

function hexWord(value) {
    return `0x${value.toString(16).padStart(4, '0')}`;
}

function verifyBytes(address, expected) {
    for (let index = 0; index < expected.length; ++index) {
        const actual = address.add(index).readU8();
        if (actual !== expected[index]) {
            throw new Error(
                `instruction mismatch at ${address.add(index)}: ` +
                `${hexByte(actual)} != ${hexByte(expected[index])}`
            );
        }
    }
}

if (Process.arch !== EXPECTED_ARCH) {
    throw new Error(`unexpected process architecture: ${Process.arch}`);
}

if (!Process.mainModule.base.equals(EXPECTED_IMAGE_BASE)) {
    throw new Error(`unexpected image base: ${Process.mainModule.base}`);
}

verifyBytes(GLYPH_ENTRY, [0x8b, 0x44, 0x24, 0x04]);
verifyBytes(ptr('0x00436974'), [0xc2, 0x08, 0x00]);

Interceptor.attach(Module.getExportByName('gdi32.dll', 'TextOutA'), {
    onEnter(args) {
        const glyph = activeGlyphs.get(this.threadId);
        if (glyph === undefined) {
            return;
        }

        try {
            const faceBuffer = Memory.alloc(64 * 2);
            const metrics = Memory.alloc(56);
            const faceLength = getTextFaceW(args[0], 64, faceBuffer);
            const metricsValid = getTextMetricsA(args[0], metrics) !== 0;
            if (faceLength <= 0 || !metricsValid) {
                throw new Error('GetTextFaceW/GetTextMetricsA failed');
            }

            const selection = {
                type: 'font-selection',
                renderer: glyph.renderer.toString(),
                width: glyph.width,
                height: glyph.height,
                face_name: faceBuffer.readUtf16String(),
                tm_height: metrics.readS32(),
                tm_ascent: metrics.add(4).readS32(),
                tm_descent: metrics.add(8).readS32(),
                tm_internal_leading: metrics.add(12).readS32(),
                tm_external_leading: metrics.add(16).readS32(),
                tm_average_char_width: metrics.add(20).readS32(),
                tm_maximum_char_width: metrics.add(24).readS32(),
                tm_weight: metrics.add(28).readS32(),
                tm_overhang: metrics.add(32).readS32(),
                tm_pitch_and_family: metrics.add(51).readU8(),
                tm_charset: metrics.add(52).readU8(),
            };
            const selectionKey = JSON.stringify(selection);
            if (!reportedFonts.has(selectionKey)) {
                reportedFonts.add(selectionKey);
                send(selection);
            }
        } catch (error) {
            send({
                type: 'capture-error',
                message: `cannot query selected GDI font: ${error}`,
            });
        }
    },
});

Interceptor.attach(GLYPH_ENTRY, {
    onEnter(args) {
        const renderer = this.context.ecx;
        const rawString = args[0];
        const destination = args[1];
        const width = renderer.add(RENDERER_WIDTH).readS32();
        const height = renderer.add(RENDERER_HEIGHT).readS32();
        const rowBytes = renderer.add(RENDERER_ROW_BYTES).readS32();
        const rawBytes = [
            rawString.readU8(),
            rawString.add(1).readU8(),
            rawString.add(2).readU8(),
        ];

        if (width <= 0 || width > MAXIMUM_GLYPH_DIMENSION ||
            height <= 0 || height > MAXIMUM_GLYPH_DIMENSION ||
            rowBytes !== Math.floor((width + 7) / 8)) {
            send({
                type: 'capture-error',
                message: 'invalid renderer geometry',
                renderer: renderer.toString(),
                width,
                height,
                row_bytes: rowBytes,
            });
            this.glyph = null;
            return;
        }

        this.glyph = {
            renderer,
            destination,
            rawBytes,
            width,
            height,
            rowBytes,
        };
        activeGlyphs.set(this.threadId, this.glyph);
    },

    onLeave() {
        activeGlyphs.delete(this.threadId);
        const glyph = this.glyph;
        if (glyph === null) {
            return;
        }

        const maskByteCount = glyph.height * glyph.rowBytes;
        const mask = glyph.destination.readByteArray(maskByteCount);
        if (mask === null) {
            send({
                type: 'capture-error',
                message: 'cannot read completed mask',
                destination: glyph.destination.toString(),
                mask_bytes: maskByteCount,
            });
            return;
        }

        const cacheKey = glyph.rawBytes[0] | (glyph.rawBytes[1] << 8);
        send({
            type: 'glyph-mask',
            renderer: glyph.renderer.toString(),
            cache_key: hexWord(cacheKey),
            raw_bytes: glyph.rawBytes.map(hexByte).join(''),
            consumed_bytes: glyph.rawBytes[0] < 0x80 ? 1 : 2,
            width: glyph.width,
            height: glyph.height,
            row_bytes: glyph.rowBytes,
            mask_bytes: maskByteCount,
        }, mask);
    },
});

send({
    type: 'ready',
    image_base: Process.mainModule.base.toString(),
    glyph_entry: GLYPH_ENTRY.toString(),
});
