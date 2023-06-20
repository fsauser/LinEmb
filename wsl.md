Pour utiliser WSL, il faut que plusieurs fonctionnalités soient activées.
Utiliser la commande suivante dans votre PowerShell:
```console
dism.exe /online /enable-feature /featurename:Microsoft-Hyper-V-All /featurename:VirtualMachinePlatform /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
```
Pour enlever le problène des espaces dans le PATH, il faut créer un fichier <**/etc/wsl.conf**> avec le contenu suivant:

```console
[interop]
appendWindowsPath = false
```
