## BIG TODO: inventory system

## SMALL TODOS: (ordered)

4. ability to move items around
5. add inventories to serialization
6. decide what to do when removing entities 
   - either entity_item that you can pick up, or
   - straight to inventory

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

- working conveyors
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
