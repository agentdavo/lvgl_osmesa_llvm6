#ifndef DX8GL_GOLDEN_IMAGE_UTILS_H
#define DX8GL_GOLDEN_IMAGE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace dx8gl {

// Simple PPM image structure
struct PPMImage {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data; // RGB data
    
    bool IsValid() const { return width > 0 && height > 0 && !data.empty(); }
    
    // Get pixel at (x, y)
    void GetPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            r = g = b = 0;
            return;
        }
        int idx = (y * width + x) * 3;
        r = data[idx];
        g = data[idx + 1];
        b = data[idx + 2];
    }
    
    // Set pixel at (x, y)
    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int idx = (y * width + x) * 3;
        data[idx] = r;
        data[idx + 1] = g;
        data[idx + 2] = b;
    }
};

// Golden image utilities
class GoldenImageUtils {
public:
    // Get the golden images directory path
    static std::string GetGoldenDir() {
        // Look for golden directory relative to test executable
        std::filesystem::path golden_dir = std::filesystem::current_path() / "ext" / "dx8gl" / "test" / "golden";
        if (!std::filesystem::exists(golden_dir)) {
            // Try relative to project root
            golden_dir = std::filesystem::current_path().parent_path() / "ext" / "dx8gl" / "test" / "golden";
            if (!std::filesystem::exists(golden_dir)) {
                // Create if doesn't exist
                std::filesystem::create_directories(golden_dir);
            }
        }
        return golden_dir.string();
    }
    
    // Generate filename for a golden image
    static std::string GetGoldenFilename(const std::string& test_name, 
                                        const std::string& scene_name,
                                        const std::string& backend) {
        return GetGoldenDir() + "/" + test_name + "_" + scene_name + "_" + backend + ".ppm";
    }
    
    // Save PPM image to file
    static bool SavePPM(const std::string& filename, const PPMImage& image) {
        if (!image.IsValid()) return false;
        
        FILE* fp = fopen(filename.c_str(), "wb");
        if (!fp) return false;
        
        // PPM header
        fprintf(fp, "P6\n%d %d\n255\n", image.width, image.height);
        
        // Write pixel data
        fwrite(image.data.data(), 1, image.data.size(), fp);
        fclose(fp);
        
        return true;
    }
    
    // Load PPM image from file
    static bool LoadPPM(const std::string& filename, PPMImage& image) {
        FILE* fp = fopen(filename.c_str(), "rb");
        if (!fp) return false;
        
        // Read header
        char header[3];
        if (fscanf(fp, "%2s", header) != 1 || strcmp(header, "P6") != 0) {
            fclose(fp);
            return false;
        }
        
        // Skip comments
        int c = fgetc(fp);
        while (c == '#') {
            while (fgetc(fp) != '\n');
            c = fgetc(fp);
        }
        ungetc(c, fp);
        
        // Read dimensions
        if (fscanf(fp, "%d %d", &image.width, &image.height) != 2) {
            fclose(fp);
            return false;
        }
        
        // Read max value
        int maxval;
        if (fscanf(fp, "%d", &maxval) != 1 || maxval != 255) {
            fclose(fp);
            return false;
        }
        
        // Skip single whitespace
        fgetc(fp);
        
        // Read pixel data
        size_t pixel_count = image.width * image.height * 3;
        image.data.resize(pixel_count);
        size_t read = fread(image.data.data(), 1, pixel_count, fp);
        fclose(fp);
        
        return read == pixel_count;
    }
    
    // Convert framebuffer to PPM image
    static PPMImage FramebufferToPPM(const void* framebuffer, int width, int height, 
                                     bool is_bgra = false, bool flip_y = false) {
        PPMImage image;
        image.width = width;
        image.height = height;
        image.data.resize(width * height * 3);
        
        const uint8_t* fb = static_cast<const uint8_t*>(framebuffer);
        
        for (int y = 0; y < height; y++) {
            int src_y = flip_y ? (height - 1 - y) : y;
            for (int x = 0; x < width; x++) {
                int src_idx = (src_y * width + x) * 4;
                int dst_idx = (y * width + x) * 3;
                
                if (is_bgra) {
                    image.data[dst_idx] = fb[src_idx + 2];     // R
                    image.data[dst_idx + 1] = fb[src_idx + 1]; // G
                    image.data[dst_idx + 2] = fb[src_idx];     // B
                } else {
                    image.data[dst_idx] = fb[src_idx];         // R
                    image.data[dst_idx + 1] = fb[src_idx + 1]; // G
                    image.data[dst_idx + 2] = fb[src_idx + 2]; // B
                }
            }
        }
        
        return image;
    }
    
    // Compare two images and compute difference metrics
    struct ComparisonResult {
        bool matches = false;
        double max_pixel_diff = 0.0;     // Maximum difference in any channel (0-255)
        double avg_pixel_diff = 0.0;     // Average difference across all pixels
        double rmse = 0.0;               // Root mean square error
        int different_pixel_count = 0;   // Number of pixels that differ
        double different_pixel_ratio = 0.0; // Ratio of different pixels
        
        // Check if difference is within tolerance
        bool IsWithinTolerance(double max_allowed_diff = 5.0, 
                              double max_diff_ratio = 0.01) const {
            return max_pixel_diff <= max_allowed_diff && 
                   different_pixel_ratio <= max_diff_ratio;
        }
    };
    
    static ComparisonResult CompareImages(const PPMImage& img1, const PPMImage& img2,
                                         double tolerance = 0.0) {
        ComparisonResult result;
        
        if (img1.width != img2.width || img1.height != img2.height) {
            result.max_pixel_diff = 255.0;
            result.different_pixel_ratio = 1.0;
            return result;
        }
        
        double total_diff = 0.0;
        double total_squared_diff = 0.0;
        int diff_count = 0;
        
        for (int y = 0; y < img1.height; y++) {
            for (int x = 0; x < img1.width; x++) {
                uint8_t r1, g1, b1, r2, g2, b2;
                img1.GetPixel(x, y, r1, g1, b1);
                img2.GetPixel(x, y, r2, g2, b2);
                
                double dr = std::abs(r1 - r2);
                double dg = std::abs(g1 - g2);
                double db = std::abs(b1 - b2);
                
                double max_channel_diff = std::max({dr, dg, db});
                
                if (max_channel_diff > tolerance) {
                    diff_count++;
                    result.max_pixel_diff = std::max(result.max_pixel_diff, max_channel_diff);
                }
                
                double pixel_diff = (dr + dg + db) / 3.0;
                total_diff += pixel_diff;
                total_squared_diff += pixel_diff * pixel_diff;
            }
        }
        
        int total_pixels = img1.width * img1.height;
        result.different_pixel_count = diff_count;
        result.different_pixel_ratio = static_cast<double>(diff_count) / total_pixels;
        result.avg_pixel_diff = total_diff / (total_pixels * 3);
        result.rmse = std::sqrt(total_squared_diff / (total_pixels * 3));
        result.matches = (diff_count == 0);
        
        return result;
    }
    
    // Generate difference image for visualization
    static PPMImage GenerateDiffImage(const PPMImage& img1, const PPMImage& img2,
                                      double amplification = 10.0) {
        PPMImage diff;
        diff.width = img1.width;
        diff.height = img1.height;
        diff.data.resize(img1.width * img1.height * 3);
        
        if (img1.width != img2.width || img1.height != img2.height) {
            // Fill with red if dimensions don't match
            std::fill(diff.data.begin(), diff.data.end(), 255);
            return diff;
        }
        
        for (int y = 0; y < img1.height; y++) {
            for (int x = 0; x < img1.width; x++) {
                uint8_t r1, g1, b1, r2, g2, b2;
                img1.GetPixel(x, y, r1, g1, b1);
                img2.GetPixel(x, y, r2, g2, b2);
                
                // Amplify differences for visualization
                int dr = std::min(255, static_cast<int>(std::abs(r1 - r2) * amplification));
                int dg = std::min(255, static_cast<int>(std::abs(g1 - g2) * amplification));
                int db = std::min(255, static_cast<int>(std::abs(b1 - b2) * amplification));
                
                diff.SetPixel(x, y, dr, dg, db);
            }
        }
        
        return diff;
    }
    
    // Check if golden image exists
    static bool GoldenExists(const std::string& filename) {
        return std::filesystem::exists(filename);
    }
    
    // Update golden image (for when rendering intentionally changes)
    static bool UpdateGolden(const std::string& filename, const PPMImage& image) {
        // Backup old golden if it exists
        if (GoldenExists(filename)) {
            std::string backup = filename + ".backup";
            std::filesystem::copy_file(filename, backup, 
                                      std::filesystem::copy_options::overwrite_existing);
        }
        
        return SavePPM(filename, image);
    }
};

// Macro for golden image comparison in tests
#define EXPECT_IMAGE_MATCHES_GOLDEN(actual_image, test_name, scene_name, backend, tolerance, max_diff_ratio) \
    do { \
        std::string golden_file = GoldenImageUtils::GetGoldenFilename(test_name, scene_name, backend); \
        if (!GoldenImageUtils::GoldenExists(golden_file)) { \
            if (::getenv("UPDATE_GOLDEN_IMAGES")) { \
                ASSERT_TRUE(GoldenImageUtils::UpdateGolden(golden_file, actual_image)) \
                    << "Failed to create golden image: " << golden_file; \
                std::cout << "Created new golden image: " << golden_file << std::endl; \
            } else { \
                GTEST_SKIP() << "Golden image not found: " << golden_file \
                            << "\nRun with UPDATE_GOLDEN_IMAGES=1 to create it"; \
            } \
        } else { \
            PPMImage golden_image; \
            ASSERT_TRUE(GoldenImageUtils::LoadPPM(golden_file, golden_image)) \
                << "Failed to load golden image: " << golden_file; \
            auto result = GoldenImageUtils::CompareImages(actual_image, golden_image, 0.0); \
            if (!result.IsWithinTolerance(tolerance, max_diff_ratio)) { \
                std::string diff_file = golden_file + ".diff.ppm"; \
                auto diff_image = GoldenImageUtils::GenerateDiffImage(actual_image, golden_image); \
                GoldenImageUtils::SavePPM(diff_file, diff_image); \
                std::string actual_file = golden_file + ".actual.ppm"; \
                GoldenImageUtils::SavePPM(actual_file, actual_image); \
                FAIL() << "Image does not match golden:\n" \
                      << "  Max pixel diff: " << result.max_pixel_diff << " (tolerance: " << tolerance << ")\n" \
                      << "  Different pixels: " << result.different_pixel_count \
                      << " (" << (result.different_pixel_ratio * 100) << "%)\n" \
                      << "  RMSE: " << result.rmse << "\n" \
                      << "  Golden: " << golden_file << "\n" \
                      << "  Actual saved to: " << actual_file << "\n" \
                      << "  Diff saved to: " << diff_file << "\n" \
                      << "  Run with UPDATE_GOLDEN_IMAGES=1 to update golden"; \
            } \
        } \
    } while(0)

} // namespace dx8gl

#endif // DX8GL_GOLDEN_IMAGE_UTILS_H