# Media Station X Server
Media Station X Server is a server that provides information and media content by using the json interface needed by the Media Station X applications.

Media Station X application is available for Mobile and Smart TVs based on Android.

The server acts in a similar way to a DLNA server but it works much better in combination with the client application Media Station X.

This server is built in C and it works only in Linux. It's light and very fast. I can serve large media content as it supports HTTP range requests allowing streaming and seeking very fast.

## Installation
The server installation is quite simple but you need to allow the server deamon to access your media content by using a user with the right permissions. This is made by calling the `make config` command wich gathers the current user and inserts it into the service file configuration before installation.

Type the following commands:

```bash
$ git clone https://github.com/vsalvans/media_station_x.github
$ cd media_station_x
$ make config
$ sudo make install

```

## Configuration
The configuration file is located at `/etc/media/media.cnf` and it comes with a basic example configuration which you have to change
This file has a simple format and it does not support comments and complex structures so be patient if the configuration parser fails.
The following comments in the configuration is just for explanation purposes but no comments are allowed yet.

```bash
server {
        title: My Media
        host: 192.168.1.100
        port: 8080
        media_buffer: 49999024
        
        location family { <-- after the keyword location you configure a media library which slug is the text after "location"
                name: My fotos
                media_dir: /media/my-photos
                file_types: jpg, jpeg, png
                display: grid
                list_type: default
                list_depth: 4
        }
}
```
You can add as many location as you want. Each location will be showin as a different menu entry.

It can be only one server.

### Server properties
| Property     | Description                                                | Options |
|--------------|------------------------------------------------------------|---------|
| title        | the title shown in the Media Station X APP starting page   | string  |
| host         | the public server's IP address, so APPs can't access to    | ip4 address  |
| port         | the server's HTTP port                                     | port number  |
| media_buffer | it's the buffer size of each packet the server sends       | integer, default 49999024  |


### Location properties
| Property     | Description                                   | Options |
|--------------|-----------------------------------------------|---------|
| name         | the menu entry and page title of the media library | string |
| media_dir    | the actual media root path | string |
| file_types   | the file types you want to show in the library media | file extensions separated by comma |
| display      | the way you want to display each media item. It can be 'grid' or 'list' | "grid", "list" |
| list_type    | the way the server will scan the media showing folders as default or recursively in a flat list usign the "flat" option | "flat", "normal" |
| list_depth   | the depth of folders you want the server to scan when usign the list_type flat option. This parameter is only required if the list_type is "flat" | number, default 4 |

