# UE4_EOS_Plugin
A plugin and sample project, built for Unreal Engine 4, that implements the Epic Online Services SDK

This plugin does not include the EOS (Epic Online Services) SDK.  You will have to register and download this separately.  Then copy the Bin, Include and Lib folders to the Plugins/Source/ThirdParty/EOSSDK directory.

The project currently supports SDK **v1.6** and Engine version **4.25.0**.

Make sure to right click the .uproject, Generate Visual Studio files and compile.

If you download and run the sample project, you will need to:
- Have registered your project on the EOS site (Dev Portal)
- Retrieved your ProductId, SandboxId and DepolymentId
- Add the ProductId, SandboxId and DeploymentId's to the Settings in Project Settings -> UEOS

In the sample, within the Player Controller, there are several input events connected to simple actions.
- E will attempt to initialise the EOS SDK, passing your parameters.
- S will attempt to shutdown the EOS SDK.

(Obviously you will want/need to move this to something more appropriate for your project.)

Once you have the SDK initialized, then you can use:
- B will initialize a Metric.
- N will attempt to Begin a Player Session.
- M will attempt to End a Player Session.

The options for Account Login/Logout will **ONLY** work if you configure your Project with Epic Account Services, through the Dashboard. Once you have done that, create a Client, get the ClientID and Client Secret, add these to the Project Settings and you're good to go. The same goes for running the Developer Authentication Tool.
Once you have initialized the SDK, you can also use:
- I will attempt to Login
- O will attempt to Logout (will only work if a successful Login occurred first)

The first/initial implementation for support of UserInfo queries is now in. You must have your project configured through EAS (in the Dashboard). You need to initialize the SDK and Login, so a valid AccountId can be retrieved.
Once you have initialized and logged in, you can also use:
- R will attempt to request UserInfo and then populate the DisplayName in the Widget

For right now, all your friend info will display. Presence not entirely working yet.

# Connect
After you login normally through Dev Tool/Account Portal, it will attempt to log you in with Steam/Epic Games. Epic Games currently generates a token, but the ContinuanceToken becomes null. Steam may end up working but I can't test it because I don't have a test app working. 

The project, blueprint nodes and comments have more direct usage information.

License:
Provided "as is."  So feel free to use it in any and all of your own projects.  Use it as a "jumping off point" to extend, fix and included into anything else you want.
(I only ask that, if you do find it useful and fix/add something - please consider adding back to the plugin with a pull request!)
