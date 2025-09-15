using OpenTK.Graphics.OpenGL;
using OpenTK.Windowing.GraphicsLibraryFramework;


namespace Gup
{
    internal unsafe class Manager
    {
        public static Window* Initialization()
        {
            Console.WriteLine("Gup Initialized");
            GLFW.Init();
            VideoMode* mode = GLFW.GetVideoMode(GLFW.GetPrimaryMonitor());
            int screenWidth = mode->Width;
            int screenHeight = mode->Height;

            GLFW.WindowHint(WindowHintBool.Decorated, true);
            GLFW.WindowHint(WindowHintBool.Resizable, true);
            GLFW.WindowHint(WindowHintBool.TransparentFramebuffer, true);


            Window* window = GLFW.CreateWindow(screenWidth, screenHeight, "Gup Window", null, null);

            GLFW.MakeContextCurrent(window);
            GL.LoadBindings(new GLFWBindingsContext());

            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

            //GLFW.MaximizeWindow(window);

            return window;
        }
    }
}
