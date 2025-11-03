import random

numWalls = 1000
numVPs = 500
numDisps = 250
numSprites = 500
numLights = 10


xml = f"""
<?xml version="1.1" encoding="UTF-8"?>

<!-- Metadata -->
<meta>
	<sky skyboxTexture="default-skybox" />
	<sun sunDirection="0.0 0.0 1.0" sunColour="1.0 1.0 1.0" sunIntensity="0.75" />
	<physics gravity="0.486" killPlaneZ="-16.0" />
	<player startPoint="-3.5 -3.5 1.0" startAngle="0.0" initialHealth="MAX" initialEnergy="MAX" />
</meta>


<!-- Environmental Objects -->
<environment>
	<visplane start="{5.0 + (numVPs/10)} 10.0" end="-10.0 -5.0" height="0.0" texture="planks" textureScale="1.5 1.5" />


	<!-- Excessive -->
"""


for wIdx in range(numWalls):
	xml += f'	<wall start="{wIdx / 100} 1.0 0.0" end="{wIdx / 100} 2.0 {1.0 + (wIdx/100)}" texture="quake" />\n'


for vIdx in range(numVPs):
	xml += f'	<visplane start="{vIdx/10} -2.0" end="{(vIdx+1)/10} -1.0" height="0.5" texture="quake" useWorldUVX="FALSE" />\n'


for dIdx in range(numDisps):
	xml += f'	<displacement aPos="{-dIdx/10} -0.5 0.5" bPos="{(1-dIdx)/10} 0.5 0.5" cPos="{(2-dIdx)/10} 0.0 1.0" />'

xml += """
</environment>

<objects>
"""

for sIdx in range(numSprites):
	xml += f'	<sprite position="{(numWalls/100) + 1.0 + (sIdx/100)} 1.5 0.5" width="1.0" height="1.0" texture="piloten" collision="FALSE" />\n'


for lIdx in range(numLights):
	xml += f'	<light position="{(lIdx/2)} 4.0 0.5" colour="{random.random()} {random.random()} {random.random()}" inputPTR="ALWAYS" intensity="2.5" />\n'
	xml += f'	<sprite position="{(lIdx/2)} 4.0 0.5" width="1.0" height="1.0" texture="lamp" collision="FALSE" />\n'

xml += """
</objects>
"""
with open(r"C:\Users\User\Documents\GitHub\Raycasting-Renderer\stages\stressnice.xml", "w") as stressFile:
	stressFile.write(xml)
