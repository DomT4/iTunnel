using System;
using System.Runtime.InteropServices;

namespace RamdiskBuilder {
    public static class Hfs {
        
        [DllImport("dmglib.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void hfslib_close(IntPtr ctx);
        
        [DllImport("dmglib.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr hfslib_open([MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport("dmglib.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool hfslib_untar(IntPtr ctx, [MarshalAs(UnmanagedType.LPStr)]string tarFile);

        [DllImport("dmglib.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern UInt64 hfslib_getsize(IntPtr ctx);

        [DllImport("dmglib.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool hfslib_extend(IntPtr ctx, UInt64 newSize);

    }
}