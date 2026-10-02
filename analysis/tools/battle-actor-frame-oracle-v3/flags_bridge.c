/* Frida 16.5.1 ia32 only. See guminterceptor-x86.c CPU_FLAGS offset. */
#include <gum/guminterceptor.h>

typedef char ContextMustHaveNineDwords[
    sizeof(GumCpuContext) == 36 ? 1 : -1];

typedef struct
{
    guint32 sequence;
} FrameInvocation;

extern guint32 frame_enter(GumCpuContext * cpu, guint32 flags,
    guint thread_id, gpointer return_address);
extern void frame_leave(GumCpuContext * cpu, guint32 flags,
    guint thread_id, guint32 sequence);

static guint32
saved_flags(GumCpuContext * cpu)
{
    /* This is Frida's saved PUSHFD word, not an invented CpuContext field. */
    return *((guint32 *) (((guint8 *) cpu) + sizeof(GumCpuContext)));
}

void
on_enter(GumInvocationContext * invocation)
{
    FrameInvocation * data =
        gum_invocation_context_get_listener_invocation_data(invocation,
            sizeof(FrameInvocation));
    GumCpuContext * cpu = invocation->cpu_context;
    data->sequence = frame_enter(cpu, saved_flags(cpu),
        gum_invocation_context_get_thread_id(invocation),
        gum_invocation_context_get_return_address(invocation));
}

void
on_leave(GumInvocationContext * invocation)
{
    FrameInvocation * data =
        gum_invocation_context_get_listener_invocation_data(invocation,
            sizeof(FrameInvocation));
    GumCpuContext * cpu = invocation->cpu_context;
    if (data->sequence != 0)
    {
        frame_leave(cpu, saved_flags(cpu),
            gum_invocation_context_get_thread_id(invocation), data->sequence);
    }
}
