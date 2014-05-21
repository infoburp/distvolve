convert input.gif frame.png

for f in *.png; do
./distvolve l $f b 8 s rendered.$f  
done

ffmpeg -i %05d.render.png output.mp4
