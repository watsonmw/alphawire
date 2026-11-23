
Bump pyawire/pyproject.toml version if not already done:

    [project]
    ...
    version = "0.1.2"

Tag current commit:
    
    git tag -a v0.1.2 -m "Release version 0.1.2"
    
Push tag to origin:

    git push origin v0.1.2


The ```.github/workflows/pypi.yml``` workflow will automatically build and publish a new version of pyawire to PyPI when a
new tag is pushed to the repository.

The [alphawireui-windows.yml](.github/workflows/alphawireui-windows.yml) workflow will build the windows UI.  Attach this to the release on Github.
