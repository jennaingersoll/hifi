You can build content in High Fidelity that breaks the laws of physical boundaries by making them oversized or extremely small. Using this tutorial, you can create a button to shrink the size of avatars in your domain. You can modify it to shrink or enlarge avatars, so that they will fit into spaces of any size.

To dynamically shrink the size of your avatar, you will need to:

- [Create Your Button](#create-your-button)
- [Write a Script](#write-a-script)
- [Connect the Script to Your Entity](#connect-the-script-to-your-entity)

## Create Your Button

First, you need to create a button that will shrink your avatar. For simple buttons, you can get away with using cube or spherical entities. However, you can make them as complex as you like by creating and importing a [3D model](https://docs.highfidelity.com/create/3d-models) into your domain.

No matter what type of entity you use, it muts be [triggerable](https://docs.highfidelity.com/create/entities/entity-behavior.html#set-an-entity-to-trigger-scripts). This lets users "push" the button using their hand controllers (in VR mode) or their mouse (in Desktop).

1. [Create an entity](create-entities.html) to be used as a button. 
2. Go to the 'Properties' tab and scroll down to the 'Behavior' section.
3. Check 'Triggerable'.

Here, we've a cylindrical entity to serve as the button to shrink your avatar.

![](scale-buttons.png)

## Write a Script

In order to work correctly, you must write a script that will define what the buttons do when they're pressed. This will be a [client entity script](https://docs.highfidelity.com/script/client-entity-scripts.html), meaning that it is attached to a entity and will run separately for each user (or client) that presses the button.

Your script will do the following:

* Set the scale factor for each button
* Resize the avatar when a button is pressed

[Download entity-client-script-shrink-avatar.js here!](https://hifi-scripting-101.glitch.me/entity-client-script-shrink-avatar.js)

### What Does the Script Do? 

To write this script, we use these namespaces and functions from our [JavaScript API](https://apidocs.highfidelity.com):

#### MyAvatar.scale Property

The `MyAvatar.scale` property sets and returns the scale (or size) of the avatar, in relation to its original size. Here, we use this property to set the desired size of the avatar. 

```javascript
    var size = 0.1;
    
    function resizeAvatar() {
        MyAvatar.scale = size;
    };
```


## Connect the Script to Your Entity
Now that you've written your script, you will need to attach it to your entity:

1. In the Create app, open the Properties window.
2. Go to the 'Behavior' section, and paste the URL to your script in the 'Script' field. If you want to use our script, copy and paste `https://hifi-scripting-101.glitch.me/entity-client-script-shrink-avatar.js` into this field.
3. Exit the **Create** app. 

Now, when either trigger the button with your hand controllers (in VR mode) or click on the button with your mouse (in Desktop mode), your avatar will shrink to 1/10 its original size. 
![](resize-avatar.gif)
