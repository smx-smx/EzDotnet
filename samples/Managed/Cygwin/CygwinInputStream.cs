using System;
using System.IO;
using System.Linq;

namespace ManagedSample
{
	public class CygwinInputStream : Stream
	{
		private readonly int fd;

		public CygwinInputStream(int fd) {
			this.fd = fd;
		}

		public override bool CanRead => true;

		public override bool CanSeek => false;

		public override bool CanWrite => false;

		public override long Length => throw new System.NotSupportedException();

		public override long Position { get => throw new System.NotSupportedException(); set => throw new System.NotImplementedException(); }

		public override void Flush()
		{
			throw new System.NotSupportedException();
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			return (int)Cygwin.Read(fd, buffer, offset, count);
		}

		public override long Seek(long offset, SeekOrigin origin)
		{
			throw new System.NotSupportedException();
		}

		public override void SetLength(long value)
		{
			throw new System.NotSupportedException();
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			throw new System.NotSupportedException();
		}
	}
}