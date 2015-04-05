using System;

public class Sample {
	public Sample () {
	}

	public void run () {
		Console.WriteLine ("Hello World");
	}

	static int Main (string[] args) {
		Sample sample = new Sample ();
		sample.run ();
		return 0;
	}
}
