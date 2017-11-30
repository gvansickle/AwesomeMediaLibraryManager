function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows")
    {
        console.log("Creating additional shortcuts");

        component.addOperation("CreateShortcut",
                               "@TargetDir@/AwesomeMediaLibraryManager.exe",
                               "@StartMenuDir@/AwesomeMediaLibraryManager.lnk");

        component.addOperation("CreateShortcut",
                               "@TargetDir@/AwesomeMediaLibraryManagerSetup.exe",
                               "@StartMenuDir@/" + qsTr("Uninstall Awesome Media Library Manger", "Start menu entry") + ".lnk",
                               "workingDirectory=@TargetDir@", "iconPath=%SystemRoot%/system32/SHELL32.dll",
                               "iconId=2", "description=Uninstall the Awesome Media Library Manger program");
    }
}
