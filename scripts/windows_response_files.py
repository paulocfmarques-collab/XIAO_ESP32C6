"""Keep GCC response files available for the full Windows build."""

import hashlib
import os
from pathlib import Path
import tempfile

import SCons.Errors
import SCons.Platform
from SCons.Platform import TempFileMunge as OriginalTempFileMunge
from SCons.Subst import quote_spaces

Import("env")


class ProjectTempFileMunge(OriginalTempFileMunge):
    # SCons recognizes substitution callables by these exact parameter names.
    def __call__(self, target, source, env, for_signature):
        command = super().__call__(target, source, env, for_signature)
        if for_signature or not isinstance(command, list):
            return command
        if len(command) != 2 or not command[1].startswith("@"):
            return command

        original = Path(command[1][1:].strip('"'))
        response_dir = Path(env.subst("$BUILD_DIR")) / "response_files"
        try:
            contents = original.read_bytes()
            response_dir.mkdir(parents=True, exist_ok=True)
            # Include command arguments and target in the name so parallel
            # compiler jobs cannot overwrite one another's response files.
            targets = target if isinstance(target, (list, tuple)) else [target]
            target_names = "\n".join(str(node) for node in targets)
            identity = target_names.encode("utf-8") + b"\0" + contents
            name = hashlib.sha256(identity).hexdigest() + ".rsp"
            destination = response_dir / name
            if original != destination:
                pending = None
                try:
                    with tempfile.NamedTemporaryFile(dir=response_dir, delete=False) as stream:
                        pending = Path(stream.name)
                        stream.write(contents)
                    os.replace(pending, destination)
                finally:
                    if pending is not None:
                        pending.unlink(missing_ok=True)
        except OSError as error:
            raise SCons.Errors.UserError(
                f"Cannot prepare GCC response file {original}: {error}"
            ) from error

        # Modify the cached command list too, so print and execution use the
        # same persistent file. SCons cleans up only its original temp file.
        command[1] = quote_spaces("@" + destination.as_posix())
        return command


if os.name == "nt":
    # The ESP32 framework imports this class after pre scripts run and sets
    # TEMPFILE again. Register here so framework and library clones use it.
    SCons.Platform.TempFileMunge = ProjectTempFileMunge
    env.Replace(TEMPFILE=ProjectTempFileMunge)
