function Component()
{
}

Component.prototype.createOperations = function()
{
    // Call the base implementation to actually install the package.
    component.createOperations();

    if (systemInfo.productType === "windows")
    {
        console.log("Registering creatiion of additional shortcuts");

        component.addOperation("CreateShortcut",
                               "@TargetDir@\\AwesomeMediaLibraryManager\\AwesomeMediaLibraryManager.exe",
                               "@StartMenuDir@\\AwesomeMediaLibraryManager.lnk");

        component.addOperation("CreateShortcut",
                               "@TargetDir@\\maintenancetool.exe",
                               "@StartMenuDir@\\UninstallAwesomeMediaLibraryManger.lnk");

        console.log("Shortcut creation registered.");
    }
}

