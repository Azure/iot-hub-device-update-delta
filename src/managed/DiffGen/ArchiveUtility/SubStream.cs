/**
 * @file SubStream.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace ArchiveUtility
{
    public class SubStream : Stream, IDisposable
    {
        private long startOffset;
        private long length;
        private Stream parentStream;
        private long parentStreamInitialPosition;

        public SubStream(Stream parentStream, long offset, long length)
        {
            this.parentStream = parentStream;
            this.startOffset = offset;
            this.length = length;
            parentStreamInitialPosition = parentStream.Position;

            parentStream.Seek(startOffset, SeekOrigin.Begin);
        }

        public new void Dispose()
        {
            parentStream.Seek(parentStreamInitialPosition, SeekOrigin.Begin);
        }

        public override bool CanRead { get { return parentStream.CanRead && ParentStreamPositionInBounds(); } }

        public override bool CanSeek { get { return parentStream.CanSeek; } }

        public override bool CanWrite { get { return parentStream.CanWrite && ParentStreamPositionInBounds(); } }

        public override long Length { get { return length; } }

        public bool ParentStreamPositionInBounds()
        {
            return ParentStreamPositionInBounds(parentStream.Position);
        }

        private bool ParentStreamPositionInBounds(long position)
        {
            long endOffset = startOffset + length;
            return position >= startOffset && position <= endOffset;
        }

        private int GetAvailableBytesToRead(int tryToRead)
        {
            if (tryToRead < 0 )
            {
                throw new Exception($"Tried to read a negative amount from stream: {tryToRead}");
            }

            long offsetAfterRead = (long) Position + tryToRead;

            if (offsetAfterRead <= Length)
            {
                return tryToRead;
            }

            return (int)(Length - Position);
        }

        public override long Position
        { 
            get 
            {
                if (!ParentStreamPositionInBounds())
                {
                    throw new Exception($"Parent stream position, {parentStream.Position}, not in bounds of substream.  Offset: {startOffset}, Length: {length}");
                }

                return parentStream.Position - startOffset;
            }
            set
            {
                Seek(value, SeekOrigin.Begin);
            }
        }
        public override void Flush()
        {
            throw new NotImplementedException();
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            int toRead = GetAvailableBytesToRead(count);
            return parentStream.Read(buffer, offset, toRead);
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            long newParentPosition;
            switch (origin)
            {
                case SeekOrigin.Begin:
                    newParentPosition = offset;
                    break;
                case SeekOrigin.Current:
                    newParentPosition = parentStream.Position + offset;
                    break;
                case SeekOrigin.End:
                    newParentPosition = parentStream.Length + offset;
                    break;
                default:
                    throw new Exception($"Unexpectged SeekOrigin type: {origin}");
            }

            if (!ParentStreamPositionInBounds(newParentPosition))
            {
                throw new Exception("Attempted to seek out of bounds of substream");
            }

            return parentStream.Seek(newParentPosition, SeekOrigin.Begin);
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            parentStream.Write(buffer, offset, count);
        }
    }
}
