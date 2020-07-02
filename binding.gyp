{
    "targets": [{
        "target_name": "module",
        "sources": [ "./module.c" ],
        "libraries": ["-laiousb", "-lusb-1.0"],
#         "ldflags": [
#                         "-Wl,-z,defs"
#                     ]
    }]
}