# a list of todos that are already done

## 1. BIG TODO: togglable light bulb (in preparation of first level)(requires interactions with objects, lights, shadows)

1. redo the file structure?
2. bounding_box_from_model(ModelHandle model) or hardcode bounding boxes(why not both?)
   - could specify in the file format if the bounding box is hardcoded or if i want to load it from the model itself
   - because i might want invisible entities, so then i would need to hardcode it
3. read entities and keymap from files(custom file format)
4. change texture(?) of light bulb between lit and unlit when interacting
5. improve the movement system
   - when moving into a bad position and actually pressing two keys at the same time(W and D over an edge),
   - it should move you in the direction of the possible movement, and not stop the movement completely
   - also when moving into a bad position it shouldnt just stop me if the next position will be bad,
   - it should allow me to move the furthest i can
   - in the platform layer instead of setting move\_\* to a specific value, add it instead so if i press A and D at the same time i actually stay in one place
   - the movement should be smoother, not starting at full speed, and not finishing instantly
   - also finally fix the camera movement, make WASD not change the y position and only change that via SPACE/SHIFT
6. player rotation
   - will change automatically depending on which way you moved
7. interpolate everything(or switch to fixed frame rate idk yet, probably this) so its smooth
8. resizability
9. figure out stripping debug features from release builds
   - or maybe dont strip them?
10. phong/blinn-phong lighting
11. shadow mapping
12. instancing
13. entity tint
    - just a value the color will be multiplied by
    - will be used for chaning the on/off state of the light bulb
    - and for possible changing the color of the ground

## 2. BIG TODO: core improvements

1. global error list
2. new mega struct entity system?
3. tests
   - unit tests for base
4. serializer
5. new renderer
   - new api
   - free gpu resources
   - static models survive the freeing
6. move away from the fixed frame rate
   - fixed tick time ("Fix Your Timestep!" article)
7. rewrite to get lsp working? (no unity build)

## 3. BIG TODO: platform layer rewrite

1. rewrite the whole main file to be platform agnostic
2. introduce threading capabilities(threads, mutexes, atomics)

## 4. BIG TODO: audio

1. write a spsc queue
2. do the audio(multithreaded)

## 5. BIG TODO: basic ui

1. renderer refactor
   - combine cpu and gpu assets
   - asset loading through the AssetManager (AssetManager loads cpu part, constructors do gpu part)
   - store static models and shaders in RenderData
   - store RenderData on the Renderer
   - make asset handles their own distinct type
   - get rid of the weird AssetType thingies
   - get rid of render pass types
2. 2D texture rendering
3. minimal usable ui
   - different sizing properties
   - vertical/horizontal layout directions
   - text rendering
   - hover/click detection
4. somewhat usable ui
   - padding and gaps
   - child alignment
   - textures
5. almost completely usable ui
   - clipping/scrolling
   - round corners
