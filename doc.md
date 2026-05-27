## BIG TODO: placing/destroying something on the ground

## SMALL TODOS: (ordered)

1. add new block types (conveyor and storage)
2. block placing/destroying mechanic
3. check collisions with already existing entities
4. ui to select which block to place
5. serialization of the world

---

## NEXT BIG TODOS: (unordered)

### Code/Engine:

- debug tools
  - map/entity/keymap editor
  - time scaling
  - system for in game tests
- profiler
- logging
- multithreading
  - if i find the need for it
- more ui improvements
  - animations

### Game Mechanics:

- inventory system
- better collision detection

### Rendering:

- particles
- more on lighting
  - pcf or other technique for better shadow mapping
  - gamma correction
  - hdr?
  - bloom?
  - ssao
  - deferred shading?
  - somehow get rid of the visible rings coming from the light bulb
  - directional light with shadow mapping
- proper font rendering

### Misc:

- get rid of sdl3(big future)

- read the projection [matrix article](https://www.songho.ca/opengl/gl_projectionmatrix.html#fov)
- read the look at [matrix article](https://morning-flow.com/2023/02/06/the-math-behind-the-lookat-transform/)
