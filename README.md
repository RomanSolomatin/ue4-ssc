# SideScroller Camera Plugin for UE4
SideScroller Camera Plugin for Unreal Engine 4

Note that this plugin is still in a very early stage and in heavy developement.

Things in place:
* Camera starting settings (Follow or static, initial camera arm length, initial camera rotation, interpolation speed, z-axis offset)
* Follow one or more Actors with camera


Planned things for v0.1 release:
* Update camera settings when actors with FollowComponent overlapping with Triggers with OverlapComponent
* Integrate Multiplayer including one Screen and Splitscreen
* Option for ignoring z-axis movement of actors (e.g. for ignore jumping)
* Integrate x,y,z-Axis limitations (e.g. for hidden rooms or level-endings)
* Integrate spline-oriented camera rotation for movement in 3-dimensional levels
* Replace the Side Scroller Camera with a Component
* Establish naming conventions for c++ files
* Implement automated Testing
* Create a Wiki

Ongoing: Update Example Map and Wiki with implemented features

## Adding The Plugin

1. Close the Unreal Editor.
1. Copy the SideScrollerCameraPlugin folder to Plugins folder next to your .uproject file.
1. Start the Unreal Editor.
1. Enable the plugin in Edit > Plugins > SideScrollerCameraPlugin.

## Usage

1. Create a player pawn without a camera or blend over to the Side Scrolle Camera for example in the Level Blueprint and add Side Scroller Follow Component to the player pawn.
1. Add the Side Scroller Camera to your level.
1. Set Up the Side Scroller Camera with your settings.
1. For Follow-Settings: Put the Side Scroller Follow Component on every actor you wish to follow with the camera.

## License

Side Scroller Camera Plugin for Unreal Engine 4 is dual-licensed:

* everything beneath Source\SideScroller\Content\StarterContent is governed by the [Unreal® Engine End User License Agreement](https://www.unrealengine.com/eula) 
* everything else is released under the MIT License
