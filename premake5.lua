-- @TODO
-- Enable SIMD
-- Maybe make netcode and realiable submodules and just link to their premake projects.
-- Maybe even do the same with raylib and glfw3...
-- I think I probably don't link to sodium on linux, just make sure it's installed? Or do I bundle it too...

workspace "WizardGame"
   configurations { "Debug", "Release" }
   architecture "x86_64"
   location "build"
   language "C"
   symbols "On"
   includedirs { "3rdparty/include" }

   filter "system:windows"
      system "windows"
      libdirs { "3rdparty/lib/windows" }

   filter "system:macosx"
      system "macosx"

   filter "system:linux"
      system "linux"

project "WizardClient"
   kind "ConsoleApp"
   files { "src/wizard_client.c", "3rdparty/include/netcode.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      links { "glfw3", "raylib", "sodium-debug" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Speed"
      links { "glfw3", "raylib", "sodium-release" }

project "WizardServer"
   kind "ConsoleApp"
   files { "src/wizard_server.c", "3rdparty/include/netcode.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      links { "sodium-debug" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Speed"
      links { "sodium-release" }
   
if os.is "windows" then
   newaction
   {
      trigger = "solution",
      description = "Create an open vs solution",
      execute = function()
         os.execute "premake5 vs2017"
         os.execute "start build/WizardGame.sln"
      end
   }
end