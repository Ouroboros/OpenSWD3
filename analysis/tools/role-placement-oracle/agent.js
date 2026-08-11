'use strict';

const EXPECTED_ARCH = 'ia32';
const EXPECTED_IMAGE_BASE = ptr('0x00400000');
const ROLE_DRAW_ENTRY = ptr('0x00413910');
const OUTPUT_FILE = 'role-placement-oracle.tsv';
const TARGET_GUIDS = new Set([248, 249]);
const SAMPLE_INTERVAL_MS = 100;
const lastSampleByGuid = new Map();

function verifyBytes(address, expected) {
    for (let index = 0; index < expected.length; ++index) {
        const actual = address.add(index).readU8();
        if (actual !== expected[index]) {
            throw new Error(
                `instruction mismatch at ${address.add(index)}: ` +
                `${actual.toString(16)} != ${expected[index].toString(16)}`
            );
        }
    }
}

function signed32(value) {
    return value | 0;
}

function hex32(value) {
    return `0x${value.toString(16).padStart(8, '0')}`;
}

if (Process.arch !== EXPECTED_ARCH) {
    throw new Error(`unexpected process architecture: ${Process.arch}`);
}

if (!Process.mainModule.base.equals(EXPECTED_IMAGE_BASE)) {
    throw new Error(`unexpected image base: ${Process.mainModule.base}`);
}

verifyBytes(ROLE_DRAW_ENTRY, [0x83, 0xec, 0x08, 0x53, 0x55, 0x56]);

const output = new File(OUTPUT_FILE, 'w');
output.write(
    'sample_ms\tguid\trole_address\tworld_x\tworld_y\trole_offset_x\t' +
    'role_offset_y\tflags\ttalk_script_id\taction_id\tbase_variant\t' +
    'variant_delta\tdraw_offset_x\tdraw_offset_y\tresource_id\t' +
    'frame_index\tmode_flags\tcamera_left\tcamera_top\t' +
    'destination_x\tdestination_y\n'
);
output.flush();

Interceptor.attach(ROLE_DRAW_ENTRY, {
    onEnter(args) {
        const role = args[0];
        const guid = role.add(0x24).readU16();
        if (!TARGET_GUIDS.has(guid)) {
            return;
        }

        const now = Date.now();
        const previous = lastSampleByGuid.get(guid);
        if (previous !== undefined && now - previous < SAMPLE_INTERVAL_MS) {
            return;
        }
        lastSampleByGuid.set(guid, now);

        const worldX = role.add(0x04).readU32();
        const worldY = role.add(0x08).readU32();
        const flags = role.add(0x10).readU32();
        const roleOffsetX = role.add(0x28).readU16();
        const roleOffsetY = role.add(0x2a).readU16();
        const action = role.add(0x40);
        const drawOffsetX = action.add(0x10).readU32();
        const drawOffsetY = action.add(0x14).readU32();
        const cameraLeft = ptr('0x004AB980').readS32();
        const cameraTop = ptr('0x004AB984').readS32();
        const destinationX = signed32(
            worldX + roleOffsetX - drawOffsetX - cameraLeft
        );
        const destinationY = signed32(
            worldY + roleOffsetY - drawOffsetY - cameraTop
        );

        const values = [
            now,
            guid,
            role.toString(),
            signed32(worldX),
            signed32(worldY),
            roleOffsetX,
            roleOffsetY,
            hex32(flags),
            role.add(0x1e).readU16(),
            signed32(action.add(0x00).readU32()),
            signed32(action.add(0x08).readU32()),
            signed32(action.add(0x34).readU32()),
            signed32(drawOffsetX),
            signed32(drawOffsetY),
            action.add(0x4a).readU16(),
            action.add(0x4c).readU16(),
            hex32(action.add(0x18).readU32()),
            cameraLeft,
            cameraTop,
            destinationX,
            destinationY,
        ];
        output.write(`${values.join('\t')}\n`);
        output.flush();
    },
});

send({
    type: 'ready',
    image_base: Process.mainModule.base.toString(),
    glyph_entry: ROLE_DRAW_ENTRY.toString(),
    output_file: OUTPUT_FILE,
});
