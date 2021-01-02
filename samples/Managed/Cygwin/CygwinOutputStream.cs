using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ManagedSample
{
	public class CygwinOutputStream : Stream
	{
		private const int BUFFER_SIZE = 4096;

		private readonly int fd;

		private int written = 0;
		private byte[] buffer;
		private MemoryStream mem;

		public CygwinOutputStream(int fd) {
			this.fd = fd;
			this.buffer = new byte[BUFFER_SIZE];
			this.mem = new MemoryStream(buffer);
		}

		public override bool CanRead => false;
		public override bool CanSeek => false;
		public override bool CanWrite => true;

		public override long Length {
			get {
				throw new NotImplementedException();
			}
		}

		public override long Position {
			get {
				throw new NotImplementedException();
			}

			set {
				throw new NotImplementedException();
			}
		}

		public override void Flush() {
			Cygwin.Write(fd, this.buffer, 0, this.written);
			this.written = 0;
			mem.Position = 0;
		}

		public override int Read(byte[] buffer, int offset, int count) {
			throw new NotImplementedException();
		}

		public override long Seek(long offset, SeekOrigin origin) {
			throw new NotImplementedException();
		}

		public override void SetLength(long value) {
			throw new NotImplementedException();
		}

		public override void Write(byte[] buffer, int offset, int count) {
			int max = BUFFER_SIZE - this.written;
			int toWrite = Math.Min(count, max);

			mem.Write(buffer, offset, toWrite);
			this.written += toWrite;

			if (buffer.Contains((byte)'\n')) {
				Flush();
			} else if (toWrite < count) {
				Flush();
				int remaining = count - toWrite;
				mem.Write(buffer, offset + toWrite, remaining);
				this.written += remaining;
			}
		}
	}
}
