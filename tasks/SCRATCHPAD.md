when i create a vector level, draw a closed shape using the brush and fill it using a bucket, if i pass an eraser over the shape it will erase the line i drew with the brush and cancel the filled area,. I wanted to have a fill behavior so that when i run the eraser it will not cancel the fill becase the 












there are a couple of level types:
    toonz raster level
    vector level
    raster level
    note level
    assistants level
    scan level

When accessing the top bar menu on Level -> New we get the options:
    New Level
    New toonz raster level
    New vector level
    New raster level
    New note level

    Which lacks some of the level types.

Also when accesing the new level dialog, either via the Level -> New -> New Level menu option or clicking the right mouse button on a x-sheet cell and selecting new level, the type dropdown on the new level dialog lacks some of the types

explore the codebase and make sure that both paths show all level types.














Add a feature for pasting test directly into the cell of a note level, sometimes we might have written the test on another document and want to paste it quickly. Remember to make it so that if a user tries to paste test on a cell diferent from a note level a warning dialog appears explaining that this can only be done for a note level.








The tool properties panel seems to be incomplete, and what is its difference from the tool option bar

















