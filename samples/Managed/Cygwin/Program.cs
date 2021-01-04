using System;
using System.IO;
using System.Runtime.InteropServices;

namespace ManagedSample
{
	public class EntryPoint
	{
		private static string[] ReadArgv(IntPtr args, int sizeBytes){
			int nargs = sizeBytes / IntPtr.Size;
			string[] argv = new string[nargs];
			
			for(int i=0; i<nargs; i++, args += IntPtr.Size) {
				IntPtr charPtr = Marshal.ReadIntPtr(args);
				argv[i] = Marshal.PtrToStringAnsi(charPtr);
			}
			return argv;
		}

		private static int Entry(IntPtr args, int sizeBytes){
			string[] argv = ReadArgv(args, sizeBytes);
			using (var stdin = new StreamReader(new CygwinInputStream(0)))
			using (var stdout = new StreamWriter(new CygwinOutputStream(1)))
			using (var stderr = new StreamWriter(new CygwinOutputStream(2))) {
				Console.SetIn(stdin);
				Console.SetOut(stdout);
				Console.SetError(stderr);

				Main(argv);
			}
			return 0;
		}

		public static void Main(string[] args)
		{
			Console.WriteLine("Hello");

			for(int i=0; i<args.Length; i++) {
				Console.WriteLine($"args[{i}] => {args[i]}");
			}

			Console.WriteLine("Write Something: ");
			Console.Out.Flush();

			string line = Console.ReadLine();
			Console.WriteLine($"You typed: {line}");

			string posix = Cygwin.ToPosixPath(@"C:\Windows\explorer.exe");
			Console.WriteLine(posix);

			Console.WriteLine("Bye");
		}
	}
}
