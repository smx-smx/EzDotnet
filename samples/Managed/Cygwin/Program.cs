using System;
using System.IO;

namespace ManagedSample
{
	public class EntryPoint
	{

		public static int Entry(IntPtr args, int sizeBytes){
			using (var stdin = new StreamReader(new CygwinInputStream(0)))
			using (var stdout = new StreamWriter(new CygwinOutputStream(1)))
			using (var stderr = new StreamWriter(new CygwinOutputStream(2))) {
				Console.SetIn(stdin);
				Console.SetOut(stdout);
				Console.SetError(stderr);

				Console.WriteLine("Hello");

				Console.WriteLine("Write Something: ");
				string line = Console.ReadLine();
				Console.WriteLine(line);

				string posix = Cygwin.ToPosixPath(@"C:\Windows\explorer.exe");
				Console.WriteLine(posix);

				Console.WriteLine("Bye");
			}            
			return 0;
		}

		static void Main(string[] args)
		{
			Console.WriteLine("Hello World!");
		}
	}
}
