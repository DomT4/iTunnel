using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace RamdiskBuilder {
    public static class xpwn {
    
        [DllImport("dmglib.dll", CallingConvention=CallingConvention.Cdecl)]
        public static extern Int32 xpwntool_enc_dec(
            [MarshalAs(UnmanagedType.LPStr)] string srcName, 
            [MarshalAs(UnmanagedType.LPStr)] string destName, 
            [MarshalAs(UnmanagedType.LPStr)] string templateFileName, 
            [MarshalAs(UnmanagedType.LPStr)] string ivStr,
            [MarshalAs(UnmanagedType.LPStr)] string keyStr);

        [DllImport("dmglib.dll", CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        public static extern string xpwntool_get_kbag(
            [MarshalAs(UnmanagedType.LPStr)]string fileName);
    }
}
