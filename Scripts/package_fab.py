#!/usr/bin/env python3
"""Create an engine-specific Fab source ZIP from the released plugin."""

import argparse
import json
from pathlib import Path
import re
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, help="Tested Unreal version, e.g. 5.8")
    parser.add_argument("--platform", action="append", required=True, help="Verified target platform")
    parser.add_argument("--product-id", help="Fab Publisher Portal product ID (required for submission)")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"5\.[0-9]+", args.engine):
        parser.error("--engine must be an Unreal 5 major/minor version")
    if args.product_id and not re.fullmatch(r"[0-9a-fA-F]{32}", args.product_id.replace("-", "")):
        parser.error("--product-id must be the Fab product UUID")

    plugin = Path(__file__).resolve().parents[1] / "Plugins" / "HorizonSDK"
    descriptor = json.loads((plugin / "HorizonSDK.uplugin").read_text())
    if any(platform not in descriptor["SupportedTargetPlatforms"] for platform in args.platform):
        parser.error("--platform is outside the plugin's supported target platforms")
    descriptor["EngineVersion"] = args.engine + ".0"
    descriptor["SupportedTargetPlatforms"] = list(dict.fromkeys(args.platform))
    descriptor["FabURL"] = (
        "com.epicgames.launcher://ue/Fab/product/" + args.product_id.replace("-", "")
        if args.product_id else ""
    )
    for module in descriptor["Modules"]:
        module["PlatformAllowList"] = descriptor["SupportedTargetPlatforms"]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    excluded = {"Binaries", "Build", "Intermediate", "Saved", "DerivedDataCache", ".git"}
    with zipfile.ZipFile(args.output, "x", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(plugin.rglob("*")):
            relative = path.relative_to(plugin)
            if any(part in excluded for part in relative.parts):
                continue
            if path.is_dir():
                archive.writestr("HorizonSDK/" + relative.as_posix() + "/", "")
            elif path.name not in {".DS_Store", ".gitkeep"}:
                if relative == Path("LICENSE"):
                    continue
                name = "HorizonSDK/" + relative.as_posix()
                if len(name) > 170:
                    raise ValueError("Fab plugin path exceeds 170 characters: " + name)
                data = (json.dumps(descriptor, indent=4) + "\n").encode() if path.name == "HorizonSDK.uplugin" else path.read_bytes()
                # Fab distributes our own code under the listing's selected license.
                # Keep copyright attribution, without imposing the repository's MIT terms.
                if relative.parts[0] == "Source" and path.suffix in {".h", ".cpp", ".cs", ".mm"}:
                    data = data.replace(
                        b"// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.",
                        b"// Copyright (c) 2026 ProjectMakers.",
                        1,
                    )
                elif relative == Path("README.md"):
                    data = data.replace(
                        b"## License\n\nThis project is licensed under the MIT License. See [LICENSE](LICENSE) for details.\n",
                        b"",
                    )
                elif relative == Path("Config/FilterPlugin.ini"):
                    data = data.replace(b"/LICENSE\n", b"")
                archive.writestr(name, data)
    print(args.output)
    if not args.product_id:
        print("Draft package only: set --product-id to populate FabURL before submission.")


if __name__ == "__main__":
    main()
