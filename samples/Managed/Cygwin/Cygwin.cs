using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ManagedSample
{
	public enum CygwinPathConversion
	{
		CCP_POSIX_TO_WIN_A = 0,
		CCP_ABSOLUTE = 0,

		CCP_POSIX_TO_WIN_W = 1,
		CCP_WIN_A_TO_POSIX = 2,
		CCP_WIN_W_TO_POSIX = 3,
		CCP_RELATIVE = 0x100,
		CCP_PROC_CYGDRIVE = 0x200,
	}


	public class Cygwin
	{
		[DllImport("cygwin1", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr cygwin_conv_path(
			CygwinPathConversion what, IntPtr from, IntPtr to, IntPtr size
		);

		[DllImport("cygwin1", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr write(int fd, IntPtr buf, IntPtr count);

		public static long Write(int fd, byte[] data, int offset, int length) {
			var gch = GCHandle.Alloc(data, GCHandleType.Pinned);
			try {
				return write(fd, gch.AddrOfPinnedObject() + offset, new IntPtr(length)).ToInt64();
			} finally {
				gch.Free();
			}
		}

		public static string ToPosixPath(string path) {
			var fromPtr = Marshal.StringToHGlobalAnsi(path);
			string result;
			try {
				var flags = CygwinPathConversion.CCP_WIN_A_TO_POSIX | CygwinPathConversion.CCP_ABSOLUTE;

				var bufSize = cygwin_conv_path(flags, fromPtr, IntPtr.Zero, IntPtr.Zero);
				var buffer = Marshal.AllocHGlobal(bufSize.ToInt32());
				try {
					cygwin_conv_path(flags, fromPtr, buffer, bufSize);
					result = Marshal.PtrToStringAnsi(buffer);
				} finally {
					Marshal.FreeHGlobal(buffer);
				}
			} finally {
				Marshal.FreeHGlobal(fromPtr);
			}
			return result;
		}
	}
}
