## CodeGhost

Instrument for monitoring integrity of files in Linux systems. Detects unauthorized changes to important files and notifies the administrator for review and approval. All events are written in log file with timestamps.
### How it works

1. Before starting the configuration file `config.json` is read 
2. Program watch all files from `watch_pathes` and calculates the SHA-256/xxHash hash of each file during initialization
3. Before watching hash differences, program checks metadata, such as time and file size. If the file is touched by user, compares files with the references hashes from storage `verified_hashes.json`
4. If new hash was detected with the changed status, it is added to the file `pending.json` and then send for review by the administrator
5. Administrator approves the change — new hash becomes the reference
6. All events are logged with the timestamp

### Requirements

- Linux
- CMake 3.16+
- C++ 17
- OpenSSL <!-- if sha256 --->
- nlohmann/json

### Build

```
git clone https://github.com/anonymmized/CodeGhost.git
cd CodeGhost
cmake -B build
cmake --build build
```

### Usage

``` bash
# with default config
./build/sanalyzer

# with specified config
./build/sanalyzer --config=config/config.json

# start in the background
./build/sanalyzer --config=config/config.json --daemonise
```

When launched with the `--daemonise` flag, the program forks, goes into the background and finally prints its PID. Standard streams are redirected to `/dev/null`. Errors are logged through the `syslog`.

### Configuration

`config/config.json`

``` json
{
	"watch_pathes": ["./blabla"],
	"ignore_pathes": ["./blabla"],
	"log_path": ["./blabla"],
	"start_hour": 9,
	"end_hour": 18,
	"watch_recursive": true
}
```

### Structure of the project

```
CodeGhost/
├── config/
│   └── critical_files.json    # list of important files
├── data/
│   ├── verified_hashes.json   # reference hashes
│   └── pending.json           # queue for approval
├── logs/
│   └── logs.log               # log of all events
└── src/
    ├── cli/                   # argument parser of command line
    ├── core/                  # main logic of hash comparison
    ├── storage/               # work with data storage
    ├── notifier/              # event notify for sending to admin
    └── utils/                 # logger and specific utils
```

### Logging

```
06.05.26 12:00:01 [INFO]  sanalyzer started.
06.05.26 12:00:01 [INFO]  Config is loaded. Recursive: 1
06.05.26 12:00:02 [WARN]  ./blabla changed.
06.05.26 12:00:02 [ERROR] Failed to open logfile.
```

### Data format

`verified_hashes.json` example:

``` json
{
  "./blabla": {
    "hash": "a1b2c3...",
    "verified_at": "2026-05-04T12:00:00Z"
  }
}
```

`pending.json` example:

``` json
{
  "entries": [
    {
      "id": "abc123",
      "path": "./blabla",
      "old_hash": "a1b2c3...",
      "new_hash": "d4e5f6...",
      "detected_at": "2026-05-06T12:00:00Z",
      "status": "pending"
    }
  ]
}
```

### License
MIT