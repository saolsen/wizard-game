-- @TODO
-- Enable SIMD
-- Maybe make netcode and realiable submodules and just link to their premake projects.
-- Maybe even do the same with raylib and glfw3...
-- I think I probably don't link to sodium on linux, just make sure it's installed? Or do I bundle it too...

-- windows builds need sodium-debug or sodium-release
-- osx and linux builds 

-- @TODO: I just need to bundle everything and build it on the fly. It's easier that way for every platform.
-- Need cross platform raylib.

workspace "WizardGame"
   configurations { "Debug", "Release" }
   architecture "x86_64"
   location "build"
   language "C"
   symbols "On"
   includedirs { "3rdparty/include" }

   configuration "Debug"
      defines { "DEBUG" }

   configuration "Release"
      defines { "NDEBUG" }
      optimize "Speed"

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
   links { "sodium", "glfw3", "raylib"}

project "WizardServer"
   kind "ConsoleApp"
   files { "src/wizard_server.c", "3rdparty/include/netcode.c" }
   links { "sodium" }
   
   
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