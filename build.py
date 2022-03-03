#!/usr/bin/env python3
import sbuildr
from sbuildr.dependencies import builders, fetchers

import glob
import os

cppstdlib = sbuildr.Library("stdc++")
libm = sbuildr.Library("m")
sfml_libs = list(map(sbuildr.Library, ["sfml-graphics", "sfml-window", "sfml-system"]))

project = sbuildr.Project()

tensor3 = sbuildr.dependencies.Dependency(
    fetchers.GitFetcher("https://github.com/pmarathe25/Tensor3"), builders.SBuildrBuilder()
)
color = sbuildr.dependencies.Dependency(
    fetchers.GitFetcher("https://github.com/pmarathe25/Color"), builders.SBuildrBuilder()
)

project.interfaces(
    filter(os.path.isfile, glob.glob(os.path.join("include", "**", "*"), recursive=True)), depends=[tensor3]
)

for source in glob.iglob(os.path.join("test", "*.cpp"), recursive=True):
    project.test(
        os.path.splitext(os.path.basename(source))[0],
        sources=[source],
        libs=[color.library("color")] + sfml_libs + [libm, cppstdlib],
    )

project.export()
