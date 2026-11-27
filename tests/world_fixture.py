"""Copy only the unpacked, versioned world into an isolated test runtime."""
import importlib.util
import os


def stage_world(repo, runtime):
    spec = importlib.util.spec_from_file_location("fixture_world", repo / "deploy/setup.py")
    setup = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(setup)
    files = setup.world_files(repo / "Genesis/area")
    for name, data in files.items():
        with (runtime / "area" / name).open("xb") as stream:
            os.fchmod(stream.fileno(), 0o600)
            stream.write(data)
    return len(files)
