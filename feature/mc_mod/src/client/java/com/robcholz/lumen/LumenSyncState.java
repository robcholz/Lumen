package com.robcholz.lumen;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.AbstractClientPlayerEntity;
import net.minecraft.client.texture.AbstractTexture;
import net.minecraft.client.texture.NativeImage;
import net.minecraft.client.texture.ResourceTexture;
import net.minecraft.client.util.SkinTextures;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.Identifier;
import net.minecraft.world.GameMode;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Base64;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public final class LumenSyncState {
    private static final Logger LOGGER = LoggerFactory.getLogger(LumenSyncState.class);

    private LumenSyncState() {
    }

    public static Snapshot requestSnapshot(boolean includeSkin) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null) {
            return Snapshot.defaultSnapshot();
        }
        CompletableFuture<Snapshot> future = new CompletableFuture<>();
        client.execute(() -> future.complete(captureSnapshot(client, includeSkin)));
        try {
            return future.get(2, TimeUnit.SECONDS);
        } catch (Exception e) {
            LOGGER.debug("Failed to capture snapshot", e);
            return Snapshot.defaultSnapshot();
        }
    }

    private static Snapshot captureSnapshot(MinecraftClient client, boolean includeSkin) {
        if (client.player == null) {
            return Snapshot.defaultSnapshot();
        }

        PlayerEntity player = client.player;
        String mode = resolveMode(client);
        double health = player.getHealth();
        double maxHealth = player.getMaxHealth();
        SkinData skin = includeSkin ? readSkin(client, player) : SkinData.empty();

        return new Snapshot(mode, health, maxHealth, skin.width(), skin.height(), skin.base64());
    }

    private static String resolveMode(MinecraftClient client) {
        if (client.interactionManager == null) {
            return "survival";
        }
        GameMode mode = client.interactionManager.getCurrentGameMode();
        if (mode == null) {
            return "survival";
        }
        return switch (mode) {
            case CREATIVE -> "creative";
            case ADVENTURE -> "adventure";
            case SPECTATOR -> "spectator";
            default -> "survival";
        };
    }

    private static SkinData readSkin(MinecraftClient client, PlayerEntity player) {
        if (!(player instanceof AbstractClientPlayerEntity clientPlayer)) {
            return SkinData.empty();
        }
        SkinTextures textures = clientPlayer.getSkinTextures();
        if (textures == null || textures.texture() == null) {
            return SkinData.empty();
        }
        AbstractTexture texture = client.getTextureManager().getTexture(textures.texture());
        if (texture instanceof ResourceTexture resourceTexture) {
            Optional<NativeImage> image = encodeResourceTexture(client, resourceTexture);
            if (image.isEmpty()) {
                return SkinData.empty();
            }
            NativeImage frontView = image.get();
            Path tempFile = null;
            try {
                tempFile = Files.createTempFile("lumen-skin-front", ".png");
                frontView.writeTo(tempFile);
                byte[] bytes = Files.readAllBytes(tempFile);
                String base64 = Base64.getEncoder().encodeToString(bytes);
                return new SkinData(frontView.getWidth(), frontView.getHeight(), base64);
            } catch (IOException e) {
                LOGGER.debug("Failed to encode front view skin", e);
                return SkinData.empty();
            } finally {
                frontView.close();
                if (tempFile != null) {
                    try {
                        Files.deleteIfExists(tempFile);
                    } catch (IOException e) {
                        LOGGER.debug("Failed to delete temp skin file", e);
                    }
                }
            }
        }
        return SkinData.empty();
    }

    private static Optional<NativeImage> encodeFrontView(NativeImage skin) {
        NativeImage front = buildFrontView(skin);
        return Optional.of(front);
    }

    private static Optional<NativeImage> encodeResourceTexture(MinecraftClient client, ResourceTexture texture) {
        Identifier location = getResourceTextureLocation(texture);
        if (location == null) {
            return Optional.empty();
        }
        Object resourceManager = client.getResourceManager();
        Object textureData = null;
        try {
            Class<?> resourceManagerClass = Class.forName("net.minecraft.resource.ResourceManager");
            Class<?> textureDataClass = Class.forName("net.minecraft.client.texture.ResourceTexture$TextureData");
            var load = textureDataClass.getDeclaredMethod("load", resourceManagerClass, Identifier.class);
            load.setAccessible(true);
            textureData = load.invoke(null, resourceManager, location);
            var getImage = textureDataClass.getDeclaredMethod("getImage");
            getImage.setAccessible(true);
            NativeImage image = (NativeImage) getImage.invoke(textureData);
            return encodeFrontView(image);
        } catch (Exception e) {
            LOGGER.debug("Failed to read resource texture {}", location, e);
            return Optional.empty();
        } finally {
            if (textureData != null) {
                try {
                    var close = textureData.getClass().getDeclaredMethod("close");
                    close.setAccessible(true);
                    close.invoke(textureData);
                } catch (Exception e) {
                    LOGGER.debug("Failed to close resource texture data", e);
                }
            }
        }
    }

    private static Identifier getResourceTextureLocation(ResourceTexture texture) {
        try {
            var field = ResourceTexture.class.getDeclaredField("location");
            field.setAccessible(true);
            return (Identifier) field.get(texture);
        } catch (ReflectiveOperationException e) {
            LOGGER.debug("Failed to access ResourceTexture location", e);
            return null;
        }
    }

    private static NativeImage buildFrontView(NativeImage skin) {
        int skinWidth = skin.getWidth();
        int skinHeight = skin.getHeight();
        boolean hasSecondLayer = skinHeight >= 64 && skinWidth >= 64;
        NativeImage front = new NativeImage(16, 32, true);

        // Head
        blit(skin, 8, 8, 8, 8, front, 4, 0);
        if (hasSecondLayer) {
            blitAlpha(skin, 40, 8, 8, 8, front, 4, 0);
        }

        // Body
        blit(skin, 20, 20, 8, 12, front, 4, 8);
        if (hasSecondLayer) {
            blitAlpha(skin, 20, 36, 8, 12, front, 4, 8);
        }

        // Right arm
        blit(skin, 44, 20, 4, 12, front, 0, 8);
        if (hasSecondLayer) {
            blitAlpha(skin, 44, 36, 4, 12, front, 0, 8);
        }

        // Left arm
        if (hasSecondLayer) {
            blit(skin, 36, 52, 4, 12, front, 12, 8);
            blitAlpha(skin, 52, 52, 4, 12, front, 12, 8);
        } else {
            blit(skin, 44, 20, 4, 12, front, 12, 8);
        }

        // Right leg
        blit(skin, 4, 20, 4, 12, front, 4, 20);
        if (hasSecondLayer) {
            blitAlpha(skin, 4, 36, 4, 12, front, 4, 20);
        }

        // Left leg
        if (hasSecondLayer) {
            blit(skin, 20, 52, 4, 12, front, 8, 20);
            blitAlpha(skin, 4, 52, 4, 12, front, 8, 20);
        } else {
            blit(skin, 4, 20, 4, 12, front, 8, 20);
        }

        return front;
    }

    private static void blit(NativeImage src, int sx, int sy, int w, int h, NativeImage dst, int dx, int dy) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int color = src.getColorArgb(sx + x, sy + y);
                dst.setColorArgb(dx + x, dy + y, color);
            }
        }
    }

    private static void blitAlpha(NativeImage src, int sx, int sy, int w, int h, NativeImage dst, int dx, int dy) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int color = src.getColorArgb(sx + x, sy + y);
                int alpha = (color >>> 24) & 0xFF;
                if (alpha == 0) {
                    continue;
                }
                dst.setColorArgb(dx + x, dy + y, color);
            }
        }
    }

    public record Snapshot(
            String mode,
            double health,
            double maxHealth,
            int skinWidth,
            int skinHeight,
            String skinBase64
    ) {
        public static Snapshot defaultSnapshot() {
            return new Snapshot("survival", 0.0, 0.0, 0, 0, "");
        }
    }

    private record SkinData(int width, int height, String base64) {
        public static SkinData empty() {
            return new SkinData(0, 0, "");
        }
    }
}
