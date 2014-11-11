using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace itunnel_csharp
{
    
    public enum RECOVERY_CALLBACK_REASON {
	    CALLBACK_RECOVERY_ENTER,
	    CALLBACK_RECOVERY_LEAVE,
	    CALLBACK_DFU_ENTER,
	    CALLBACK_DFU_LEAVE,
    };
    public static class libmd
    {
        [DllImport("libMobiledevice.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int libmd_platform_init();
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void recovery_callback_t(RECOVERY_CALLBACK_REASON reason, IntPtr device, IntPtr ctx);
        [DllImport("libMobiledevice.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int libmd_set_recovery_callback(recovery_callback_t callback, IntPtr ctx);
        [DllImport("libMobiledevice.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int libmd_builtin_uploadFile(IntPtr device, [MarshalAs(UnmanagedType.LPStr)]string fileName);
        [DllImport("libMobiledevice.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int call_AMRecoveryModeDeviceSendFileToDevice(IntPtr device, [MarshalAs(UnmanagedType.LPStr)]string fileName);
        [DllImport("libMobiledevice.dll", CallingConvention = CallingConvention.Cdecl)]
         public static extern int libmd_builtin_sendCommand(IntPtr device, [MarshalAs(UnmanagedType.LPStr)]string command);
    }
}
