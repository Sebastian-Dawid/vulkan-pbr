# vulkan-template
To initialize the git submodules use:
```bash
git submodule update --init # to initialize the includes
cd includes
git submodule update --init # to initialize imgui
```

To compile the programm in debug mode use: ```make build_debug```

To run the programm in debug mode use: ```make test```

To compile the programm in release mode use: ```make build_release```

To run the programm in release mode use: ```make release```

On MacOs add ```PORTABILITY=-DPORTABILITY``` to the ```make``` call otherwise Vulkans portability flag will not be set.
