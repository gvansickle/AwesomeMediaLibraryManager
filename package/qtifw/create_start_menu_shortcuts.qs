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
                               "@StartMenuDir@/AwesomeMediaLibraryManager.lnk",
                               "description=Run Awesome Media Library Manager");

        component.addOperation("CreateShortcut",
                               "@TargetDir@/maintenancetool.exe",
                               "@StartMenuDir@/Uninstall Awesome Media Library Manger.lnk",
                               "workingDirectory=@TargetDir@", "iconPath=%SystemRoot%/system32/SHELL32.dll",
                               "iconId=2", "description=Uninstall the Awesome Media Library Manger program");
    }
}
