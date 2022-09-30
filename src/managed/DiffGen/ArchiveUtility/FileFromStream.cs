namespace ArchiveUtility
{
    using System;
    using System.IO;

    // This class lets a user get access to a file with the content of the stream.
    // If the stream is a FileStream then we can use the backing file for that 
    // stream directly, otherwise we create a temp file and write the contents
    // of the stream to the file and delete this temporary file when 
    // we are done.
    public class FileFromStream : IDisposable
    {
        public Stream Stream { get; private set; }
        public string Name { get; private set; }

        private bool usingTempFile = false;

        public FileFromStream(Stream stream, string workingFolder = "", string ext = "")
        {
            Stream = stream;

            var fileStream = stream as FileStream;

            if (fileStream != null)
            {
                Name = fileStream.Name;
                return;
            }

            if (string.IsNullOrEmpty(workingFolder))
            {
                workingFolder = Path.GetTempPath();
            }

            usingTempFile = true;
            string tempFilePath = Path.Combine(workingFolder, Path.GetRandomFileName() + ext);
            Name = tempFilePath;

            stream.Seek(0, SeekOrigin.Begin);
            using var writeStream = File.OpenWrite(tempFilePath);
            stream.CopyTo(writeStream);
        }

        public void Dispose()
        {
            if (usingTempFile)
            {
                File.Delete(Name);
            }
        }
    }
}
