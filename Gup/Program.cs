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

            Renderer.Equals

            while (true)
            {
                if (GLFW.WindowShouldClose(window) == true)
                    break;

                GLFW.PollEvents();



            }
            GLFW.DestroyWindow(window);
            GLFW.Terminate();
        }
    }

}
