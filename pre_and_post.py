import os
import atexit
import subprocess
from pathlib import Path
from shutil import copyfile, which
#from SCons.Script import DefaultEnvironment  # type: ignore

Import("env") # type: ignore

#print("Current CLI targets", COMMAND_LINE_TARGETS)
#print("Current Build targets", BUILD_TARGETS)

# Füge hier deinen NVM-Node-Pfad ein
os.environ["PATH"] += os.pathsep + "/Users/daniel/.nvm/versions/node/v22.13.1/bin"

# Pfade
source_file = Path("lib/CPortal/CPortal.cpp")
backup_file = source_file.with_suffix(".cpp.bak")
web_dir = Path("web")
html_file = Path("web/dist/index.html")

# Platzhalter im C++-Code
placeholder = "__HTML_CONTENT_WILL_BE_INJECTED__"

# Wird aufgerufen, auch wenn der Build fehlschlägt
@atexit.register
def restore_backup_on_exit():
    if backup_file.exists():
        print(">>> 🟢 Originaldatei wird wiederhergestellt: ", source_file)
        copyfile(backup_file, source_file)
        backup_file.unlink()

def run_npm_build():
    
    npm_path = which("npm")
    if not npm_path:
        print(">>> ❌ Fehler: Kann 'node' nicht finden – ist es installiert und im PATH?")
        env.Exit(1) # type: ignore
    
    print(">>> 🚀 [npm] Führe 'npm install' im ./web Verzeichnis aus...")
    result_install = subprocess.run(["npm", "install"], cwd=web_dir, capture_output=True, text=True)
    if result_install.returncode != 0:
        print(">>> ❌ Fehler: npm install fehlgeschlagen:")
        print(result_install.stdout)
        print(result_install.stderr)
        env.Exit(1) # type: ignore
    
    print(">>> 🚀 [npm] Führe 'npm run build' im ./web Verzeichnis aus...")
    result_build = subprocess.run(["npm", "run", "build"], cwd=web_dir, capture_output=True, text=True)
    if result_build.returncode != 0:
        print(">>> ❌ Fehler: npm build fehlgeschlagen:")
        print(result_build.stdout)
        print(result_build.stderr)
        env.Exit(1) # type: ignore

def before_build(source, target, env):    
    run_npm_build()
    
    if not html_file.exists():
        print(f">>> ❌ Fehler: {html_file} existiert nicht.")
        env.Exit(1) # type: ignore

    # HTML lesen, minifizieren und escapen
    raw_html = html_file.read_text(encoding="utf-8")
 
    # In C++-Quelldatei einfügen
    cpp_content = source_file.read_text(encoding="utf-8")
    if placeholder not in cpp_content:
        print(f">>> ❌ Fehler: Platzhalter '{placeholder}' nicht in {source_file}")
        env.Exit(1) # type: ignore
    
    # Backup erstellen
    copyfile(source_file, backup_file)

    cpp_modified = cpp_content.replace(placeholder, f'{raw_html}')
    source_file.write_text(cpp_modified, encoding="utf-8")

def after_upload(source, target, env):
    print(">>> 🟢 [post] Upload erfolgreich")

# Aktionen registrieren
#env.AddPreAction("upload", before_build)
before_build(None, None, env) # type: ignore
#env.AddPostAction("upload", after_upload)