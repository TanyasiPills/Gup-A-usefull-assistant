using OpenTK.Windowing.GraphicsLibraryFramework;
using OpenTK.Graphics.OpenGL4;

namespace Gup
{
    public struct RenderData
    {
        public RenderData(VertexArray vaIn, IndexBuffer ibIn, Shader sdIn, Texture txIn)
        {
            va = vaIn;
            ib = ibIn;
            shader = sdIn;
            texture = txIn;
        }

        public VertexArray va;
        public IndexBuffer ib;
        public Shader shader;
        public Texture texture;
    };

    public unsafe class Renderer
    {
        float yToX;
        int screenWidth, screenHeight;
        Window* winRef;

        public void Initialization(Window* window)
        {
            winRef = window;
            GLFW.GetFramebufferSize(window, out int sW, out int sH);
            screenWidth = sW;
            screenHeight = sH;
            GL.Viewport(0, 0, screenWidth, screenHeight);

            yToX = screenWidth / screenHeight;
        }

        void Draw(RenderData data)
        {
            data.va.Bind();
            data.shader.Bind();
            data.texture.Bind();
            GL.DrawElements(BeginMode.Triangles, data.ib.GetCount(), DrawElementsType.UnsignedInt, 0);
        }

        void Clear()
        {
            GL.ClearColor(0f, 0f, 0f, 0f);
            GL.Clear(ClearBufferMask.ColorBufferBit);
        }

        /// <summary>
        /// should do that there is a list of render data which will get rendered, the user can append and remove from that
        /// can be id hangled but this should contain somehow the concurentQueue
        /// </summary>
        void Render()
        {
            Clear();
            GLFW.SwapBuffers(winRef);
        }

    }
}
