# vulkan-template
To initialize the git submodules use: ```git submodule update --init```

To compile the programm in debug mode use: ```make build_debug```

To run the programm in debug mode use: ```make test```

To compile the programm in release mode use: ```make build_release```

To run the programm in release mode use: ```make release```

On MacOs add ```PORTABILITY=-DPORTABILITY``` to the ```make``` call otherwise Vulkans portability flag will not be set.
