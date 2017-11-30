function Component()
{
}

Component.prototype.createOperations = function()
{
    // Call default implementation to actually install the package.
    component.createOperations();

    try
    {
        if (systemInfo.productType === "windows")
        {
            console.log("Creating additional shortcuts");

            component.addOperation("CreateShortcut",
                                   "@TargetDir@\\AwesomeMediaLibraryManager\\AwesomeMediaLibraryManager.exe",
                                   "@StartMenuDir@\\AwesomeMediaLibraryManager.lnk",
                                   "description=Run Awesome Media Library Manager");

            component.addOperation("CreateShortcut",
                                   "@TargetDir@\\maintenancetool.exe",
                                   "@StartMenuDir@\\UninstallAwesomeMediaLibraryManger.lnk",
                                   "description=Uninstall the Awesome Media Library Manger program");

            console.log("Shortcuts created.");
        }
    } catch(e) {
        print(e);
        var result = QMessageBox.question("quit.question", "Installer", "Shortcut error",
                                      QMessageBox.Yes | QMessageBox.No);
    }
}

function Controller()
{
}

Controller.prototype.StartMenuDirectoryPageCallback = function()
{
    var result = QMessageBox.question("quit.question", "Installer", "Shortcut callback",
                              QMessageBox.Yes | QMessageBox.No);
}
