print("Processing playlist: " + playlist.location);

line = '';
do
{
    line = readln();
    if (line)
        print("Read line from playlist file: " + line);

} while (line);

print("Playlist script done");

