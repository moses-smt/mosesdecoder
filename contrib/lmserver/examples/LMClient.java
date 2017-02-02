import java.io.DataInputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.net.URI;
import java.net.URISyntaxException;

public class LMClient {

	private Socket sock;
	private DataInputStream input;
	private OutputStreamWriter output;

	public LMClient(URI u) throws IOException {
		sock = new Socket(u.getHost(), u.getPort());
		System.err.println(sock);
		input = new DataInputStream(sock.getInputStream());
		output = new OutputStreamWriter(sock.getOutputStream(), "UTF8");
	}

	public float wordLogProb(String word, String context) throws IOException {
		return wordLogProb(word, context.split("\\s+"));
	}

	public float wordLogProb(String word, String[] context) throws IOException {
		StringBuffer sb = new StringBuffer();
		sb.append("prob ");
		sb.append(word);
		for (int i = context.length-1; i >= 0; --i) {
			sb.append(' ').append(context[i]);
		}
		sb.append("\r\n");
		output.write(sb.toString());
		output.flush();
		byte b1 = input.readByte();
		byte b2 = input.readByte();
		byte b3 = input.readByte();
		byte b4 = input.readByte();
		Float f = Float.intBitsToFloat( (((b4 & 0xff) << 24) | ((b3 & 0xff) << 16) | ((b2 & 0xff) << 8) | (b1 & 0xff)) );
		input.readByte(); input.readByte();
		return f;
	}

	public static void main(String[] args) {
		try {
			LMClient lm = new LMClient(new URI("lm://csubmit02.umiacs.umd.edu:6666"));
			System.err.println(lm.wordLogProb("want", "<s> the old man"));
			System.err.println(lm.wordLogProb("wants", "<s> the old man"));
		} catch (URISyntaxException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
