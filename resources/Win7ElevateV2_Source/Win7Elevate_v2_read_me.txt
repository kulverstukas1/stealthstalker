This file contains supplemental detail about the proof-of-concept code described here:
 http://www.pretentiousname.com/misc/win7_uac_whitelist2.html


As far as the source goes, Win7Elevate_Inject.cpp is the most interesting file. The rest is mostly utility and GUI code of little interest, except perhaps the little utility class for copying memory from one process to another.

There is more code in the proof-of-concept app than there has to be:

- GUI code (there's probably more of that than anything else; a very small command-line version or library could be written instead)

- Error handling / reporting (helps to diagnose things if it stops working but also adds bulk to the code)

- Clean-up code (we wouldn't want to leave a mess after bypassing a security feature; that'd just be rude!)

- The ability to target differnet processes (Explorer.exe is always a suitable target but it's interesting to see what else works, e.g. Calc.exe, Notepad.exe, MSPaint.exe...)

- Probably some unused stuff from when I was experimenting with ideas.

- etc.

Don't let the amount of code fool you into thinking there's anything complex going on. There isn't really.


** Caveats: **

The following caveats are because I didn't bother making the code cope with every combination of process types. None of these things are inherently impossible but it didn't seem worth it when I only wanted to prove that my theory worked:

- Right now you can only use the 64-bit version on 64-bit Windows and the 32-bit version on 32-bit Windows.

- You can also only target 64-bit or 32-bit processes with their respective versions. If you accidentally inject a 64-bit process with the 32-bit version or vice versa then the process you inject will crash.

- The process you target probably needs to have ASLR switched on, unless you recompile the EXE with ASLR off (in which case the target needs to have ASLR off). Essentially, Kernel32.dll needs to be loaded at the same address in both processes and thus the ASLR flags must match between the two processes. All of the relevant Microsoft processes seem to have ASLR on so you can probably ignore this, but if you find the target process crashes then check the ASLR flag in Process Explorer.

(As I understand it, ASLR randomizes where kernel32.dll is loaded *per session*, not per process, so all processes running in the same session with the same ASLR flag have kernel32.dll at the same address.)

- The code is not safe for simultaneous use from multiple processes/threads. (You'd need to use a mutex or something to avoid the dummy DLL from being deleted by one process/thread while it is still needed by the other.)


** How it works in detail: **

The code exploits two flaws. IMO the first is more interesting/important and also harder to fix. The second flaw should be easy to fix, but it's probably also easy to find more flaws like it, so it's less interesting (but should still be fixed).

The first part is what I've explained on my site with some steps added to launch the second part. (Don't worry about what FileA/FolderB/ProgramC are yet.)

1.1) We select a process signed with the Windows Publisher certificate, such as Explorer.exe.
1.2) We inject code into the selected process and make it run that code on a new thread. (There are no restrictions to doing this so long as the selected process is a peer of ours; i.e. same session(I think?), user and integrity level. Explorer.exe runs at medium integrity and is always running so it's a good target.)
1.3) The injected code creates an elevated IFileOperation object. (If the Win7 defaults are in effect and the selected process is a Windows Publisher one then this does not trigger a UAC prompt.)
1.4) The injected code uses the IFileOperation object to copy FileA to FolderB.
1.5) The injected code launches ProgramC. (Doesn't have to happen in the injected code but doing it there is easier.)
1.6) The injected code waits for ProgramC to finish.
1.7) The injected code uses the IFileOperation object to delete FileA from FolderB. (Cleaning up after itself.)

The second part involves FileA, FolderB and ProgramC.

FileA is actually Win7ElevateDll(32|64).dll, an extremely simple DLL whose source is included. (dllmain.cpp is the only interesting part of the DLL.) This DLL is embedded as a resource inside the EXE and extracted as needed, so that the EXE remains a standalone program.

As soon as anything attaches to the DLL it will:

2.1) Look up the arguments that its host program was run with.
2.2) Run whatever program/command is specified by those arguments.
2.3) Kill the host program.

So if we can trick an elevated program into loading that DLL then we can run whatever we specify with elevation.

Note: We can write to Program Files and System32, however we cannot (yet) move/rename/replace many of the files in System32. This is because they're owned by TrustedInstaller and need ownership/permission changes before administrators can change them. There might be a COM object waiting to be found which can do that but we certainly don't need it yet.

So, FileA is our dummy DLL...

Originally, FolderB and ProgramC pointed to "C:\Program Files\Windows Media Player\wmpconfig.exe" which was the first item on Rafael's list of auto-elevated programs:

http://www.withinwindows.com/2009/02/05/list-of-windows-7-beta-build-7000-auto-elevated-binaries/

I figured I'd start at the top of the list and it turned out I didn't need to look any further, back when the beta (build 7000) was current.

However, wmpconfig.exe's auto-elevation status was removed in build 7022 so I had to look for another process in Rafael's updated list:

http://www.withinwindows.com/2009/05/02/short-windows-7-release-candidate-auto-elevate-white-list/

I decided to start at the bottom this time, knowing executables not directly below System32 were most likely to be suitable (I'll explain why in a moment). As before, the first exe I tried worked and I didn't need to look any further. Changing that exe path is the only modification I've had to make since the code was originally written.

For builds from 7022 until 7100 RC1 (and presumably beyond), FolderB and ProgramC point to "C:\Windows\System32\sysprep\sysprep.exe"

How did I know these exes were vulnerable? It took very little time: I loaded Process Monitor and then launched wmpconfig.exe (and later sysprep.exe) to see if it tried & failed to load any DLLs from its own directory before moving on to other directories... Indeed it did, a DLL called CRYPTBASE.DLL.

The real CRYPTBASE.DLL exists in System32 and we cannot currently replace it because of the TrustedInstaller permissions. But we can, using the first/main flaw, copy a fake CRYPTBASE.DLL to sysprep.exe's folder so that sysprep.exe loads our DLL instead of the real one.

(By default when a process loads a DLL it will look in its own folder first and fall back on System32 (etc.) if it wasn't found. Windows has a list of "Known DLLs" which will always be loaded directly from System32 without looking in the exe's own folder first. That list exists to avoid diversions like this and is a good idea. Unfortunately, Microsoft seem to have forgotten to put CRYPTBASE.DLL on the list. Oops. I would have told them this back in February if they had bothered to respond to my repeated offers to give them the full details of this stuff. Instead of putting their heads in the sand. Perhaps there's still time to add CRYPTBASE.DLL to the list and fix the second flaw for the RTM, but I imagine there are other ways to exploit the ability to copy a file to a protected folder. I found the CRYPTBASE.DLL issue in a matter of minutes and didn't bother looking for alternatives..)

So, going back to steps 1.4 and 1.5, the injected code copies our dummy DLL, Win7ElevateDll(32|64).dll, to C:\Windows\System32\sysprep\CRYPTBASE.DLL and then runs sysprep.exe, passing the details of what to run on the command line.

With the Win 7 UAC defaults, sysprep.exe (and about 70 other processes) automatically and silently elevates itself no matter who runs it. It doesn't matter that we launch sysprep.exe from a medium-integrity process; it will elevate itself to high integrity. Note that Microsoft don't allow anyone else's processes to have this ability. Apparently it's alright for Microsoft code to require admin access, without displaying a UAC prompt, but it's not alright for anyone else's code to do the same thing.

Anyway, as soon as sysprep.exe loads our DLL, the DLL takes over, looks up the process's command-line and launches what we told it to. Because sysprep.exe is elevated the thing we launch is also elevated. sysprep.exe is then killed, CRYPTBASE.DLL is deleted (we're polite and wouldn't want to break sysprep!) and we're done.


** You may be wondering... **

Do we actually need to copy the DLL to where sysprep.exe is at all? Couldn't we instead copy sysprep.exe to where the DLL is and avoid the need to write to Program Files?

No, is the answer. If you copy an auto-elevated process to another location then it will no longer auto-elevate because UAC no longer recognises its certificate. That is good thinking on Microsoft's part. The automatic/silent elevation stuff only happens for binaries in specially permissioned folders. So Microsoft clearly thought about and tried to avoid this sort of problem, they just didn't do a particularly good job of it, and then they ignored and dismissed someone who tried to tell them about something they missed.
