Portals in High Fidelity transport you to the domain of your choice. You can use these portals to travel to a domain you visit frequently instead of using the **GoTo** app on your HUD or Tablet. 

To create a portal, you will need to:

- [Create Your Portal](#create-your-portal)
- [Write a Script](#write-a-script)
- [Connect the Script to Your Entity](#connect-the-script-to-your-entity)

## Create Your Portal

First, you need to create a portal using [entities](https://docs.highfidelity.com/create/entities). For simple portals like doors or holes in the ground, you can get away with using cube or spherical entities. However, you can make them as complex as you like by creating and importing a [3D model](https://docs.highfidelity.com/create/3d-models) into your domain.

No matter what type of entity you use, it muts be [collisionless](https://docs.highfidelity.com/create/entities/entity-behavior.html#set-entity-behavior-on-collision). This lets avatars walk into the portal, and trigger the script to teleport to a new location.

1. [Create an entity](create-entities.html) to be used as a portal. 
2. Go to the 'Properties' tab and scroll down to the 'Behavior' section.
3. Add `hifi://welcome` (or the destination of your choice) to the 'User Data' field. 
4. Scroll down to the 'Collision' section and uncheck 'Collides'. By turning off collisions, avatars can walk into the entity and trigger the script. 

Here, we've created a cube entity to serve as a "door" to another domain.

![](portal-door.png)

## Write a Script

In order to work correctly, you must write a script that defines what happens when an avatar walks into your portal. This script will be a [client entity script](https://docs.highfidelity.com/script/client-entity-scripts.html), meaning that it is attached to a entity and will run separately for each avatar (or client) that walks into it.

Your script will do the following:

* Monitor activity to determine when an avatar walks into the portal
* Set a teleport destination
* Play a sound that is played each time an avatar enters the portal
* Teleport avatars to the specified destination

[Download portal.js here!](https://raw.githubusercontent.com/highfidelity/hifi-content/44a10a3fb07f3271307ef0a2c28429d51f696326/DomainContent/Home/portal.js)

### What Does the Script Do? 

To write this script, we use these namespaces and functions from our [JavaScript API](https://apidocs.highfidelity.com):

#### Entities.getEntityProperties Method
Each entity has a set of properties that defines its appearance and behavior, such as its type, color, position, and rotation. Throughout this script, we will use the `Entities.getEntityProperties` method to:

* Track the position of the avatar so that we know when it enters the portal
* Read in the teleport destination from the 'User Data' field

```javascript
    function getProps(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        if (properties) {
            position = properties.position;
            portalDestination = properties.userData;
        }
    }
```

#### Vec3 Namespace
The `Vec3` namespace defines the 3D coordinate space that High Fidelity uses in it domain. With it, you can calculate the position of an entity, avatar, or other object in a domain. Here, we used the namespace to determine whether a set of coordinates is inside portal.

```javascript
    function isPositionInsideBox(position, boxProperties) {
        var localPosition = Vec3.multiplyQbyV(Quat.inverse(boxProperties.rotation),
            Vec3.subtract(MyAvatar.position, boxProperties.position));
        var halfDimensions = Vec3.multiply(boxProperties.dimensions, 0.5);
        return -halfDimensions.x <= localPosition.x &&
            halfDimensions.x >= localPosition.x &&
            -halfDimensions.y <= localPosition.y &&
            halfDimensions.y >= localPosition.y &&
            -halfDimensions.z <= localPosition.z &&
            halfDimensions.z >= localPosition.z;
    }
```

#### Entities.enterEntity Signal
The  `Entities.enterEntity` signal is triggered whenever an avatar enters the same location as an entity. In this script, we use this signal to teleport the avatar and play a sound when it enters the portal.

```javascript
    this.enterEntity = function(entityID) {
        playSound(entityID);
        Window.location = "hifi://" + portalDestination;
    }
```

#### Audio.playSound Method
The `Audio.playSound` method plays an audio file. You can specify additional options, such as the volume, location of where the sound originates, and whether the sound is broadcasted to everyone in the domain. Here, we have created a method that plays a sound when an avatar enters a specific location.

``` javascript
    var TELEPORT_SOUND_VOLUME = 0.40;

    var teleportSound;
    var position;
    
    function playSound(entityID) {
        if (teleportSound.downloaded) {
            Audio.playSound(teleportSound, { position: position, volume: TELEPORT_SOUND_VOLUME, localOnly: true });
        }
    }
```

## Connect the Script to Your Entity
Now that you've written your script, you will need to attach it to your entity:

1. In the Create app, open the Properties window.
2. Go to the 'Behavior' section, and paste the URL to your script in the 'Script' field. If you want to use our script, copy and paste `https://raw.githubusercontent.com/highfidelity/hifi-content/44a10a3fb07f3271307ef0a2c28429d51f696326/DomainContent/Home/portal.js` into this field.
3. Exit the **Create** app. 

Now, when you walk into your portal (that you built in [Create Your Portal](#create-your-portal) with an entity), you will be transported to the destination specified in the 'User Data' field of the portal entity.

![](enter-portal.gif)
