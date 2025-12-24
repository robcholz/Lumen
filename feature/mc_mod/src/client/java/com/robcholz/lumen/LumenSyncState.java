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

import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public final class LumenSyncState {
    private static final Logger LOGGER = LoggerFactory.getLogger(LumenSyncState.class);
    private static final int SKIN_HEIGHT = 120;

    private LumenSyncState() {
    }

    public static Snapshot requestPlayerSnapshot() {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null) {
            return Snapshot.defaultSnapshot();
        }
        CompletableFuture<Snapshot> future = new CompletableFuture<>();
        client.execute(() -> future.complete(capturePlayerSnapshot(client)));
        try {
            return future.get(2, TimeUnit.SECONDS);
        } catch (Exception e) {
            LOGGER.debug("Failed to capture snapshot", e);
            return Snapshot.defaultSnapshot();
        }
    }

    public static SkinPayload requestSkinPayload() {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null) {
            return SkinPayload.empty();
        }
        CompletableFuture<SkinPayload> future = new CompletableFuture<>();
        client.execute(() -> future.complete(captureSkinPayload(client)));
        try {
            return future.get(5, TimeUnit.SECONDS);
        } catch (Exception e) {
            LOGGER.debug("Failed to capture skin payload", e);
            return SkinPayload.empty();
        }
    }

    private static Snapshot capturePlayerSnapshot(MinecraftClient client) {
        if (client.player == null) {
            return Snapshot.defaultSnapshot();
        }

        PlayerEntity player = client.player;
        String mode = resolveMode(client);
        double health = player.getHealth();
        double maxHealth = player.getMaxHealth();
        return new Snapshot(mode, health, maxHealth);
    }

    private static SkinPayload captureSkinPayload(MinecraftClient client) {
        if (client.player == null) {
            return SkinPayload.empty();
        }
        return readSkin(client, client.player);
    }

    private static String resolveMode(MinecraftClient client) {
        if (client.interactionManager == null) {
            return "-----";
        }
        GameMode mode = client.interactionManager.getCurrentGameMode();
        if (mode == null) {
            return "-----";
        }
        return switch (mode) {
            case CREATIVE -> "Creative";
            case ADVENTURE -> "Adventure";
            case SPECTATOR -> "Spectator";
            default -> "Survival";
        };
    }

    private static byte[] toRGB565(NativeImage image) {
        int width = image.getWidth();
        int height = image.getHeight();
        byte[] data = new byte[width * height * 2];
        int idx = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int color = image.getColorArgb(x, y);
                int r = (color >>> 16) & 0xFF;
                int g = (color >>> 8) & 0xFF;
                int b = color & 0xFF;
                int value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >>> 3);
                data[idx++] = (byte) (value & 0xFF);
                data[idx++] = (byte) ((value >>> 8) & 0xFF); // little-endian
            }
        }
        return data;
    }

    private static NativeImage scaleImage(NativeImage image) {
        int srcWidth = image.getWidth();
        int srcHeight = image.getHeight();
        int dstHeight = SKIN_HEIGHT;
        int dstWidth = Math.max(1, Math.round((srcWidth * (float) dstHeight) / srcHeight));
        NativeImage scaled = new NativeImage(dstWidth, dstHeight, true);
        for (int y = 0; y < dstHeight; y++) {
            int srcY = (y * srcHeight) / dstHeight;
            for (int x = 0; x < dstWidth; x++) {
                int srcX = (x * srcWidth) / dstWidth;
                int color = image.getColorArgb(srcX, srcY);
                scaled.setColorArgb(x, y, color);
            }
        }
        return scaled;
    }

    private static SkinPayload readSkin(MinecraftClient client, PlayerEntity player) {
        if (!(player instanceof AbstractClientPlayerEntity clientPlayer)) {
            return SkinPayload.empty();
        }
        SkinTextures textures = clientPlayer.getSkinTextures();
        if (textures == null || textures.texture() == null) {
            return SkinPayload.empty();
        }
        AbstractTexture texture = client.getTextureManager().getTexture(textures.texture());
        if (texture instanceof ResourceTexture resourceTexture) {
            Optional<NativeImage> image = encodeResourceTexture(client, resourceTexture);
            if (image.isEmpty()) {
                return SkinPayload.empty();
            }
            NativeImage frontView = image.get();
            NativeImage scaled = scaleImage(frontView);
            byte[] bytes = toRGB565(scaled);
            return new SkinPayload(scaled.getWidth(), scaled.getHeight(), bytes);
        }
        return SkinPayload.empty();
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
            double maxHealth
    ) {
        public static Snapshot defaultSnapshot() {
            return new Snapshot("-----", 0.0, 0.0);
        }
    }

    public record SkinPayload(int width, int height, byte[] rgb565) {
        public static SkinPayload empty() {
            return new SkinPayload(0, 0, new byte[0]);
        }

        public byte[] toWireBytes() {
            if (rgb565.length == 0) {
                return new byte[0];
            }
            byte[] data = new byte[4 + rgb565.length];
            data[0] = (byte) (width & 0xFF);
            data[1] = (byte) ((width >>> 8) & 0xFF);
            data[2] = (byte) (height & 0xFF);
            data[3] = (byte) ((height >>> 8) & 0xFF);

            for (int i = 0; i < rgb565.length; i += 2) {
                byte lsb = rgb565[i];
                byte msb = rgb565[i + 1];
                data[4 + i] = msb;
                data[4 + i + 1] = lsb;
            }
            return data;
        }
    }
}
