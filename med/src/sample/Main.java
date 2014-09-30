package sample;

import javafx.animation.AnimationTimer;
import javafx.application.Application;
import javafx.beans.property.SimpleDoubleProperty;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.control.Slider;
import javafx.scene.control.TextField;
import javafx.scene.image.Image;
import javafx.scene.image.PixelWriter;
import javafx.scene.paint.Color;
import javafx.stage.Stage;

public class Main extends Application {

    @Override
    public void start(Stage primaryStage) throws Exception{
        Parent root = FXMLLoader.load(getClass().getResource("main.fxml"));
        primaryStage.setTitle("med");
        primaryStage.setScene(new Scene(root, 1024, 768));
        primaryStage.show();

        final Slider sliderScaleX = (Slider)root.lookup("#scaleX");
        final Slider sliderScaleY = (Slider)root.lookup("#scaleY");
        final TextField textScaleX = (TextField)root.lookup("#scaleTextX");

        final SimpleDoubleProperty scaleX = new SimpleDoubleProperty();
        final SimpleDoubleProperty scaleY = new SimpleDoubleProperty();
        sliderScaleX.valueProperty().bindBidirectional(scaleX);
        sliderScaleY.valueProperty().bindBidirectional(scaleY);

        final Canvas canvas = (Canvas)root.lookup("#canvas");
        canvas.setWidth(512);
        canvas.setHeight(512);
        new AnimationTimer() {
            @Override
            public void handle(long now) {
                final GraphicsContext gc = canvas.getGraphicsContext2D();
                double w = canvas.getWidth();
                double h = canvas.getHeight();
                gc.clearRect(0, 0, w, h);

                PixelWriter writer = gc.getPixelWriter();

                for (int y = 0; y < (int)h; ++y) {
                    for (int x = 0; x < (int)w; ++x) {
                        // noise in the range [-1, 1]
                        double f = SimplexNoise.noise(x/scaleX.getValue(), y/scaleY.getValue());
                        int xx = (int)(255 * (0.5 + f/2));
                        writer.setArgb(x, y, (0xff << 24) + (xx << 16) + (xx << 8) + (xx << 0));
                    }
                }
//                gc.setFill(Color.rgb(200, 200, 50, 0.5));
//                gc.fillRect(0, 0, w, h);
            }
        }.start();
    }


    public static void main(String[] args) {
        launch(args);
    }
}
