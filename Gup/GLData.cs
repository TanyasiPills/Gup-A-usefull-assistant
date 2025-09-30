using OpenTK.Graphics.OpenGL4;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;
using System.Reflection.Metadata;

namespace Gup
{
    public struct VertexBufferElement
    {
        public uint type;
        public uint count;
        public bool normalized;

        public VertexBufferElement(uint type, uint count, bool normalized)
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

    public class  VertexBufferLayout
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
                elements.Add(new VertexBufferElement(0x1406, 1, false));
                stride += count * VertexBufferElement.GetSizeOfType(0x1406);
            } else if (cur == typeof(char) || cur == typeof(bool) || cur == typeof(byte)){
                elements.Add(new VertexBufferElement(0x1400, 1, false));
                stride += count * VertexBufferElement.GetSizeOfType(0x1400);
            }
        }

        public List<VertexBufferElement> GetElements() { return elements; }
        public uint GetStride() { return stride; }
    }
    public unsafe class VertexBuffer
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

    public class VertexArray
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
            uint offset = 0;
            for (int i = 0; i < elements.Count; i++)
            {
                var element = elements[i];
                GL.EnableVertexAttribArray(i);
                GL.VertexAttribPointer(i, (int)element.count, (VertexAttribPointerType)element.type, element.normalized, (int)layout.GetStride(), (nint)(i * VertexBufferElement.GetSizeOfType(element.type)));
                offset += element.count * VertexBufferElement.GetSizeOfType(element.type);
            }
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

    public unsafe class IndexBuffer
    {
        uint IBO;
        int count;

        public void Init(void* data, int size)
        {
            count = size;
            GL.GenBuffers(1, out IBO);
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, IBO);
            GL.BufferData(BufferTarget.ElementArrayBuffer, size, (nint)data, BufferUsageHint.StaticDraw);
        }

        ~IndexBuffer()
        {
            GL.DeleteBuffer(IBO);
        }

        public void Bind()
        {
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, IBO);
        }

        public void Unbind()
        {
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, 0);
        }

        public int GetCount() { return count; }
    }

    public class Shader    
    {
        uint shaderId = 0;
        Dictionary<string, int> uniformLocs;

        enum ShaderSourceType
        {
            Vertex,
            Fragment,
            None
        }

        ~Shader()
        {
            GL.DeleteProgram(shaderId);
        }

        public void Bind()
        {
            GL.UseProgram(shaderId);
        }

        public void Unbind()
        {
            GL.UseProgram(0);
        }


        public string[] ParseShader(string filepath)
        {
            string[] sources = new string[2];
            ShaderSourceType type = ShaderSourceType.None;

            using (StreamReader sr = new StreamReader(filepath)) 
            {
                while (!sr.EndOfStream)
                {
                    string line = sr.ReadLine();
                    if (line.Contains("#shader")) {
                        if (line.Contains("vertex")) {
                            type = ShaderSourceType.Vertex;
                        } else if (line.Contains("fragment")) {
                            type |= ShaderSourceType.Fragment;
                        }
                    }
                    else {
                        sources[(int)type] += line + "\n";
                    }
                }
            }

            return sources;
        }

        public int CompileShader(string source, ShaderType type)
        {
            int id = GL.CreateShader(type);

            GL.ShaderSource(id, source);
            GL.CompileShader(id);

            return id;
        }

        public uint CreateShader(string vertexShader, string fragmentShader)
        {
            int program = GL.CreateProgram();

            int vertexProgram = CompileShader(vertexShader, ShaderType.VertexShader);
            int fragmentProgram = CompileShader(fragmentShader, ShaderType.FragmentShader);

            GL.AttachShader(program, vertexProgram);
            GL.AttachShader(program, fragmentProgram);

            GL.LinkProgram(program);
            GL.ValidateProgram(program);

            GL.DeleteShader(vertexProgram);
            GL.DeleteShader(fragmentProgram);

            GL.UseProgram(program);

            return (uint)program;
        }

        public void BindShaderFile(string filepath)
        {
            string[] source = ParseShader(filepath);
            shaderId = CreateShader(source[(int)ShaderSourceType.Vertex], source[(int)ShaderSourceType.Fragment]);
        }

        private int GetUniformLocation(string name)
        {
            if (uniformLocs.ContainsKey(name)) return uniformLocs[name];
            else {
                int loc = GL.GetUniformLocation(shaderId, name);
                uniformLocs.Add(name, loc);
                return loc;
            }
        }

        public void SetUniform1(string name, double x)
        {
            GL.Uniform1(GetUniformLocation(name), x);
        }

        public void SetUniform2(string name, double x, double y)
        {
            GL.Uniform2(GetUniformLocation(name), x, y);
        }

        public void SetUniform3(string name, double x, double y, double z)
        {
            GL.Uniform3(GetUniformLocation(name), x, y, z);
        }

        public void SetUniform4(string name, double x, double y, double z, double q)
        {
            GL.Uniform4(GetUniformLocation(name), x, y, z, q);
        }
    }

    public class Texture
    {
        int TO = 0;
        int width = 0;
        int height = 0;
        int bpp = 0;

        ~Texture()
        {
            GL.DeleteTexture(TO);
        }

        public void Init(string path)
        {
            using (var image = Image.Load<Rgba32>(path))
            {
                width = image.Width;
                height = image.Height;

                byte[] pixels = new byte[width * height * 4];
                image.CopyPixelDataTo(pixels);

                TO = GL.GenTexture();
                GL.BindTexture(TextureTarget.Texture2D, TO);

                GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
                GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
                GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
                GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);

                GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba8, 
                    width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, pixels);

                GL.BindTexture(TextureTarget.Texture2D, 0);
            }
        }

        public void Init(byte[] data, int widthIn, int heightIn)
        {
            width = widthIn;
            height = heightIn;

            TO = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, TO);

            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.ClampToEdge);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.ClampToEdge);

            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba8, 
                width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, data);

            GL.BindTexture(TextureTarget.Texture2D, 0);
        }

        public void Bind()
        {
            GL.BindTexture(TextureTarget.Texture2D, TO);
        }

        public void Unbind()
        {
            GL.BindTexture(TextureTarget.Texture2D, 0);
        }

        public void BlendCorrection()
        {
            Bind();

            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);

            Unbind();
        }

        public void TransparencyCorrection()
        {
            Bind();

            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Nearest);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Nearest);

            Unbind();
        }
    }

}
