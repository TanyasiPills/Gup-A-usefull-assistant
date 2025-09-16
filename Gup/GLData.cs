using OpenTK.Graphics.OpenGL4;

namespace Gup
{
    struct VertexBufferElement
    {
        uint type;
        uint count;
        char normalized;

        public VertexBufferElement(uint type, uint count, char normalized)
        {
            this.type = type;
            this.count = count;
            this.normalized = normalized;
        }

        static public uint GetSizeOfType(uint type)
        {
            switch (type)
            {
                case 0x1400: // GL_BYTE
                case 0x1401: // GL_UNSIGNED_BYTE
                    return 1;
                case 0x1402: // GL_SHORT
                case 0x1403: // GL_UNSIGNED_SHORT
                    return 2;
                case 0x1404: // GL_INT
                case 0x1405: // GL_UNSIGNED_INT
                case 0x1406: // GL_FLOAT
                    return 4;
                case 0x140A: // GL_DOUBLE
                    return 8;
                default:
                    throw new ArgumentException("Unknown type");
            }
        }
    }

    internal class  VertexBufferLayout
    {
        private List<VertexBufferElement> elements;
        uint stride;

        public VertexBufferLayout()
        {
            elements = new List<VertexBufferElement>();
            stride = 0;
        }
        public void Push<T>(uint count)
        {
            Type cur = typeof(T);
            if (cur == typeof(float) || cur == typeof(uint) || cur == typeof(int))
            {
                elements.Add(new VertexBufferElement(0x1406, 1, (char)0));
                stride += count * VertexBufferElement.GetSizeOfType(0x1406);
            } else if (cur == typeof(char) || cur == typeof(bool) || cur == typeof(byte)){
                elements.Add(new VertexBufferElement(0x1400, 1, (char)0));
                stride += count * VertexBufferElement.GetSizeOfType(0x1400);
            }
        }

        public List<VertexBufferElement> GetElements() { return elements; }
        public uint GetStride() { return stride; }
    }
    internal unsafe class VertexBuffer
    {
        uint VBO;

        public VertexBuffer(void* data, int size)
        {
            GL.GenBuffers(1, out VBO);
            GL.BindBuffer(BufferTarget.ArrayBuffer, VBO);
            GL.BufferData(BufferTarget.ArrayBuffer, size, (nint)data, BufferUsageHint.StaticDraw);
        }

        ~VertexBuffer()
        {
            GL.DeleteBuffer(VBO);
        }

        public void Bind()
        {
            GL.BindBuffer(BufferTarget.ArrayBuffer, VBO);
        }

        public void Unbind()
        {
            GL.BindBuffer(BufferTarget.ArrayBuffer, 0);
        }
    }

    internal class VertexArray
    {
        uint VAO;
        VertexBufferLayout layout;

        public VertexArray()
        {
            GL.GenVertexArrays(1, out VAO);
        }

        ~VertexArray()
        {
            GL.DeleteVertexArray(VAO);
        }

        public void SetLayout(VertexBufferLayout layoutIn)
        {
            layout = layoutIn;
        }

        public void SetBuffer(ref VertexBuffer buffer)
        {
            Bind();
            buffer.Bind();
            var elements = layout.GetElements();
        }

        public void Bind()
        {
            GL.BindVertexArray(VAO);
        }

        public void Unbind()
        {
            GL.BindVertexArray(0);
        }
    }

    internal class IndexBuffer
    {
    }

    internal class Shader
    {
    }

    internal class Texture
    {
    }

}
