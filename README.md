# UE4_EOS_Plugin
A plugin and sample project, built for Unreal Engine 4, that implements the Epic Online Services SDK

This plugin does not include the EOS (Epic Online Services) SDK.  You will have to register and download this separately.  Then copy the Bin, Include and Lib folders to the Plugins/Source/ThirdParty/EOSSDK directory.

The project currently supports SDK **v1.7** and Engine version **4.25.1**.

Make sure to right click the .uproject, Generate Visual Studio files and compile.

If you download and run the sample project, you will need to:
- Have registered your project on the EOS site (Dev Portal)
- Retrieved your ProductId, SandboxId and DepolymentId

In order to test, look in the Credentials folder and see the README.txt file and instructions on what to do.

You can set various login type options in the BP_PlayerController blueprint asset.

Go to "Plugins\UEOS\Source\ThirdParty" and follow instructions there to drag various folders from the latest SDK into the project.

# Testing
When you log in to test, if you are using DevTool - by default I set the port to "12345" and then type in the credential type in the box.

Only other login I handle atm is "AccountPortal" type which is handled by your browser and Epic authentication.

It will automatically login with auth and connect sequentially as well.
