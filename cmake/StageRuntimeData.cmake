# Keep a local playtest's saved settings across rebuilds.
file(COPY "${SOURCE}/configs" DESTINATION "${DEST}" PATTERN "user" EXCLUDE)
# The runtime consumes cooked meshes and sidecars. Blender sources stay in the checkout.
file(COPY "${SOURCE}/assets" DESTINATION "${DEST}" PATTERN "source" EXCLUDE PATTERN "briefs" EXCLUDE)
