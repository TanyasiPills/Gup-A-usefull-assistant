using OpenTK;
using OpenTK.Graphics.OpenGL;
using OpenTK.Windowing.GraphicsLibraryFramework;


namespace Gup
{

    internal unsafe class Gup
    {
        static void Main(string[] args)
        {
            Window* window = Manager.Initialization();

            while (true)
            {
                if (GLFW.WindowShouldClose(window) == true)
                    break;

                GLFW.PollEvents();

                GL.ClearColor(0f, 0f, 0f, 0f);
                GL.Clear(ClearBufferMask.ColorBufferBit);
                GLFW.SwapBuffers(window);
            }
            GLFW.DestroyWindow(window);
            GLFW.Terminate();
        }
    }

}
