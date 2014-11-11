using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace itunnel_csharp
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        /// 
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            libmd.libmd_platform_init();
            libmd.libmd_set_recovery_callback(Program.recoveryCallback, IntPtr.Zero);
            Application.Run(new Form1());
        }

        public static void recoveryCallback(RECOVERY_CALLBACK_REASON reason, IntPtr device, IntPtr ctx)
        {
            switch (reason)
            {
                case RECOVERY_CALLBACK_REASON.CALLBACK_RECOVERY_ENTER:
                    libmd.call_AMRecoveryModeDeviceSendFileToDevice(device, @"\\srv\upload\ssh4\42b1\iBEC.n88ap.RELEASE.dfu");
                    //libmd.libmd_builtin_uploadFile(device, @"\\srv\upload\ssh4\42b1\iBEC.n88ap.RELEASE.dfu");
                    libmd.libmd_builtin_sendCommand(device, "go");
                    break;
                default:
                    break;
            }
            return;
        }
    }
}
