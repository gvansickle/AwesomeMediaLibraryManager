function Component()
{
}

Component.prototype.createOperations = function()
{
    // Call default implementation to actually install README.txt!

    component.createOperations();
    if (systemInfo.productType === "windows")
    {
        console.log("Creating additional shortcuts");

        component.addOperation("CreateShortcut",
                               "@TargetDir@/AwesomeMediaLibraryManager/AwesomeMediaLibraryManager.exe",
                               "@StartMenuDir@/AwesomeMediaLibraryManager.lnk",
                               "description=Run Awesome Media Library Manager");

        component.addOperation("CreateShortcut",
                               "@TargetDir@/maintenancetool.exe",
                               "@StartMenuDir@/UninstallAwesomeMediaLibraryManger.lnk",
                               "description=Uninstall the Awesome Media Library Manger program");
    }
}
