from PIL import Image
import os

def extract_tiles(image_path, tile_size=128):
    # Open the image
    img = Image.open(image_path)
    width, height = img.size

    # Create output directory if it doesn't exist
    os.makedirs(image_path.replace(".png",""), exist_ok=True)

    tile_count = 0
    # Loop over the image in tile-sized steps
    for y in range(0, height, tile_size):
        for x in range(0, width, tile_size):
            # Define the bounding box of the tile
            box = (x, y, x + tile_size, y + tile_size)
            
            # Crop the tile from the image
            tile = img.crop(box)
            
            # Save the tile as a new image
            tile_filename = os.path.join(image_path.replace(".png",""), f'tile_{tile_count}.png')
            tile.save(tile_filename)
            tile_count += 1

    print(f"{tile_count} tiles saved to '{image_path.replace('.png','')}'.")

# Example usage
extract_tiles('sheet-sprites.png')
