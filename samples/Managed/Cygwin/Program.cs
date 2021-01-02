using System;
using System.IO;

namespace ManagedSample
{
	public class EntryPoint
	{
		private static int CommonEntry(){
			using (var stdin = new StreamReader(new CygwinInputStream(0)))
			using (var stdout = new StreamWriter(new CygwinOutputStream(1)))
			using (var stderr = new StreamWriter(new CygwinOutputStream(2))) {
				Console.SetIn(stdin);
				Console.SetOut(stdout);
				Console.SetError(stderr);

				Console.WriteLine("Hello");
				Console.WriteLine("Write Something: ");
				Console.Out.Flush();

				string line = Console.ReadLine();
				Console.WriteLine(line);

				string posix = Cygwin.ToPosixPath(@"C:\Windows\explorer.exe");
				Console.WriteLine(posix);

				Console.WriteLine("Bye");
			}            
			return 0;
		}

		public static void EntryCLR() => CommonEntry();
		public static int EntryCoreCLR(IntPtr args, int sizeBytes) => CommonEntry();

		public static void Main(string[] args)
		{
			Console.WriteLine("Hello World!");
		}
	}
}
