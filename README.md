# imageshrink

imageshrink is a tool to shrink the file size of Jpeg images.
This is achieved by lowering the Jpeg quality as long as original image and the new image do not differ too much.
The difference between the original image and the new image is determined by calculating the [Structural Dissimilarity DSSIM](https://en.wikipedia.org/wiki/Structural_similarity).
The Jpeg image quality as decreased until the DSSIM value does not exceed a previously defined threshold.

## Dependencies

* libjpeg-turbo
* Apache log4cxx (optional)

## Settings
```
imageshrink [settings] inputFile outputFile

settings:
    --min value                 minimum jpeg quality (20 <= value <= 100, default = 20)
    --max value                 maximum jpeg quality (50 <= value <= 100, default = 85)
    --dssimAvgMax value         maximum for the average DSSIM (0 <= value <= 1, default = 0.0001)
    --dssimPeakMax value        maximum for the peak DSSIM (0 <= value <= 1, default = 0.001)
    --copyMarkers value         maximum for the peak DSSIM (value = none|all, default = all)
    --initQualityStep value     init value for qulaity steps (1 <= value <= 10, default = 10)
    --cs444to420 value          convert cs444 to cs420 (value = true|false, default = true)
    --imageCompChunkSize value  image chunk size for comparison (8 <= value <= 256, default = 160)
```

## License

[MIT](./LICENSE.txt)
