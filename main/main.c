#include "vmsys.h"
#include "vmpromng.h"
#include "vmtimer.h"
#include "vmsock.h"

#define TIMER_MS 3000

VMBOOL bg_done = VM_FALSE;
VMBOOL armed = VM_FALSE;
static VMINT timer_id = -1;
static VMINT tcp_handle = -1;

static void start_connection(void);

static void tcp_cb(VMINT handle, VMINT event)
{
    switch (event)
    {
    case VM_TCP_EVT_CONNECTED:
        vm_cbm_hold_bearer(VM_RES_TYPE_TCP, handle);
        break;

    case VM_TCP_EVT_PIPE_BROKEN:
    case VM_TCP_EVT_PIPE_CLOSED:
        tcp_handle = -1;
        break;

    case VM_TCP_EVT_HOST_NOT_FOUND:
        tcp_handle = -1;
        break;
    }
}

static void start_connection(void)
{
    if (tcp_handle >= 0)
        return;

    tcp_handle = vm_tcp_connect("google.com", 80, VM_TCP_APN_CMNET, tcp_cb);
}

static void stop_connection(void)
{
    vm_cbm_release_bearer();

    if (tcp_handle >= 0)
    {
        vm_tcp_close(tcp_handle);
        tcp_handle = -1;
    }
}

void timer(VMINT tid)
{
    if (tid == timer_id)
        timer_id = -1;

    armed = VM_TRUE;

    if (tcp_handle < 0)
        start_connection();

    timer_id = vm_create_timer_ex(TIMER_MS, timer);
}

void handle_sysevt(VMINT message, VMINT param)
{
    switch (message)
    {
    case VM_MSG_CREATE:
        start_connection();

        if (timer_id == -1)
        {
            timer_id = vm_create_timer_ex(TIMER_MS, timer);
        }

    break;

    case VM_MSG_PAINT:

        if (!bg_done)
        {
            bg_done = VM_TRUE;
            vm_pmng_set_bg();
        }
        else if (armed)
        {
            stop_connection();

            if (timer_id != -1)
            {
                vm_delete_timer_ex(timer_id);
                timer_id = -1;
            }

            vm_exit_app();
            return;
       }

    break;

    case VM_MSG_HIDE:
        break;

    case VM_MSG_QUIT:
        stop_connection();

        if (timer_id != -1)
        {
            vm_delete_timer_ex(timer_id);
            timer_id = -1;
        }

        break;
    }
}

void vm_main(void)
{
    vm_reg_sysevt_callback(handle_sysevt);
}
