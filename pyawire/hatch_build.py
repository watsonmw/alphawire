import glob
import os
import sys
from hatchling.builders.hooks.plugin.interface import BuildHookInterface

class CustomBuildHook(BuildHookInterface):
    def initialize(self, version, build_data):
        # root to python path so wo we can import build_extension
        sys.path.insert(0, self.root)
        import build_extension

        ffi_builder, all_sources = build_extension.setup()

        if self.target_name == 'wheel':
            ffi_builder.compile(verbose=1, tmpdir=self.root)

            for ext in glob.glob(os.path.join(self.root, 'awire', '_binding*')):
                if ext.endswith(('.so', '.pyd')):
                    build_data['artifacts'].append(os.path.relpath(ext, self.root))

            import sysconfig
            # Get the raw platform string (e.g., 'macosx-11.1-arm64')
            raw_platform = sysconfig.get_platform()
            
            # For macOS 11+, the standard compatibility tag is often 'macosx_11_0_arm64'
            # regardless of the minor version, especially for Apple Silicon.
            # Pip debug tags usually include 'macosx_11_0_arm64'.
            if raw_platform.startswith('macosx-11'):
                platform_tag = 'macosx_11_0_arm64'
            else:
                platform_tag = raw_platform.replace('-', '_').replace('.', '_')
            
            # Use abi3 tag for broad Python 3 compatibility.
            # cp37 is a safe minimum for most modern features.
            python_tag = 'cp37'
            abi_tag = 'abi3'
            build_data['tag'] = f'{python_tag}-{abi_tag}-{platform_tag}'

        else:
            ffi_builder.emit_c_code('awire/_binding.c')
